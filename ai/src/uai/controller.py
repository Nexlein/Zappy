from context import DroneContext
from utils.navigation import BROADCAST_DIRECTION_ARRIVED
from utils.config_loader import (
    get_swarm_config,
    get_evolution_config,
    get_reproduction_config,
)
from BroadcastProtocol import BroadcastProtocol, MessageType
from ai_logger import ai_logger

from uai.utilities import UtilityCalculators
from uai.actions import ActionGenerators


class UtilityAIController(UtilityCalculators, ActionGenerators):
    """
    A Utility-based AI Controller for the Zappy drone, fully synced with the FSM
    robustness features (drone_id tie-breaker, READY/LEAVING/INCANT/ABORT handshakes).
    """

    def __init__(self, context: DroneContext):
        self.context = context
        self._forward_streak = 0
        swarm_cfg = get_swarm_config()
        self.tick_since_bcast = swarm_cfg.get("BCAST_INTERVAL", 2)

        # Leader state
        self.ready_count = 0
        self.is_leader = False
        self.leader_aborted = False
        self.rally_ticks = 0

        # Incantation sequence state
        self.incant_bcast_sent = False
        self.incant_cmd_sent = False

        # Follower state
        self.is_following = False
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self.follower_leaving = False
        self.target_leader_id = ""
        self.highest_rally_direction = None

        # Reproduction state
        self.forks_done = 0
        self.reproduce_connect_sent = False
        self.reproduce_fork_sent = False
        self.reproduce_attempted = False

        self.last_level = context.level
        self.last_behavior = "survival"

    def _reset_reproduce_state(self):
        self.reproduce_connect_sent = False
        self.reproduce_fork_sent = False
        self.reproduce_attempted = True

    def _reset_leader_state(self):
        self.ready_count = 0
        self.is_leader = False
        self.leader_aborted = False
        self.rally_ticks = 0
        self.incant_bcast_sent = False
        self.incant_cmd_sent = False

    def _reset_follower_state(self):
        self.is_following = False
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self.follower_leaving = False
        self.target_leader_id = ""
        self.highest_rally_direction = None

    def _process_messages(self):
        """Process all incoming network broadcasts to update internal states."""
        if self.context.level > self.last_level:
            # We leveled up! Reset all tracking states.
            self._reset_leader_state()
            self._reset_follower_state()
            self.last_level = self.context.level

        for bcst in self.context.broadcasts:
            decoded = bcst.content
            if (
                not decoded
                or decoded.team_name != self.context.team_name
                or decoded.level != self.context.level
            ):
                continue

            # Follower listening to Leader
            if self.is_following:
                if (
                    decoded.msg_type == MessageType.ABORT
                    and decoded.drone_id == self.target_leader_id
                ):
                    self._reset_follower_state()
                elif (
                    decoded.msg_type == MessageType.INCANT
                    and decoded.drone_id == self.target_leader_id
                ):
                    if bcst.direction in BROADCAST_DIRECTION_ARRIVED:
                        self.waiting_incant = True
                    else:
                        self._reset_follower_state()

            # Leader listening to Followers / Other Leaders
            if self.is_leader:
                if (
                    decoded.msg_type == MessageType.READY
                    and bcst.direction in BROADCAST_DIRECTION_ARRIVED
                ):
                    self.ready_count += 1
                    self.rally_ticks = 0
                elif decoded.msg_type == MessageType.LEAVING and self.ready_count > 0:
                    self.ready_count -= 1
                elif (
                    decoded.msg_type == MessageType.RALLY
                    and decoded.drone_id > self.context.drone_id
                ):
                    self._reset_leader_state()
                    self.is_following = True

            # Always track the highest leader for following
            if decoded.msg_type == MessageType.RALLY:
                if decoded.drone_id >= self.target_leader_id:
                    self.target_leader_id = decoded.drone_id
                    self.highest_rally_direction = bcst.direction

    def tick(self) -> str | None:
        if self.context.ticks_since_inventory >= 15:
            self.context.ticks_since_inventory = 0
            return "Inventory"

        self.context.ticks_since_inventory += 1

        self.highest_rally_direction = None
        self._process_messages()

        evo_cfg = get_evolution_config()
        if self.context.level >= evo_cfg.get("MAX_LEVEL", 8):
            return self._get_survival_action()

        # Hard overrides for sequences that must complete before utility is evaluated
        if self.leader_aborted:
            self._reset_leader_state()
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.ABORT,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if self.follower_leaving:
            self._reset_follower_state()
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.LEAVING,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if self.waiting_incant:
            return None  # Frozen

        # New hunger cycle: clear the reproduce suppressor + any half-done
        # sequence, so the next well-fed window gets a fresh fork attempt.
        repr_cfg = get_reproduction_config()
        if self.context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            self.reproduce_attempted = False
            self.reproduce_connect_sent = False
            self.reproduce_fork_sent = False

        # Compute Utilities
        u_survival = self._get_survival_utility()
        u_incantation = self._get_incantation_utility()
        u_rally = self._get_rally_utility(u_survival)
        u_follow = self._get_follow_utility(u_survival)
        u_gather = self._get_gather_utility(u_survival)
        u_reproduce = self._get_reproduce_utility(u_survival)

        utilities = {
            "survival": u_survival,
            "incantation": u_incantation,
            "rally": u_rally,
            "follow": u_follow,
            "gather": u_gather,
            "reproduce": u_reproduce,
        }

        best_behavior = max(utilities.items(), key=lambda item: item[1])[0]

        if utilities[best_behavior] <= 0.0:
            best_behavior = "survival"

        if best_behavior != self.last_behavior:
            self.context.path_queue.clear()
            self.last_behavior = best_behavior

        # Handle state transitions based on utility
        if best_behavior == "survival":
            if self.is_leader:
                self.leader_aborted = True
                return self.tick()  # Re-evaluate to send abort
            if self.is_following and self.ready_sent:
                self.follower_leaving = True
                return self.tick()  # Re-evaluate to send leaving

        if best_behavior == "rally":
            self.is_leader = True
            self.is_following = False
            self.rally_ticks += 1
            swarm_cfg = get_swarm_config()
            if self.rally_ticks > swarm_cfg.get("RALLY_TIMEOUT", 100):
                # Timed out waiting for followers. Abort and try to reproduce.
                self.rally_ticks = 0
                self.reproduce_attempted = False
                best_behavior = "reproduce"
        elif best_behavior == "follow":
            self.is_following = True
            self.is_leader = False
        else:
            self.rally_ticks = 0

        # Execute Action
        if best_behavior == "incantation":
            action = self._get_incantation_action()
        elif best_behavior == "rally":
            action = self._get_rally_action()
        elif best_behavior == "follow":
            action = self._get_follow_action()
        elif best_behavior == "gather":
            action = self._get_gather_action()
        elif best_behavior == "reproduce":
            action = self._get_reproduce_action()
        else:
            action = self._get_survival_action()

        ai_logger.log_state(
            best_behavior,
            action or "None",
            self.context.level,
            self.context.inventory.food,
        )
        return action
