from context import DroneContext
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType
from utils.navigation import BROADCAST_DIRECTION_ARRIVED
from utils.config_loader import (
    get_swarm_config,
    get_reproduction_config,
    get_evolution_config,
)
from ai_logger import ai_logger

from uai.state import UAIState
from uai.calculator import UtilityCalculator
from uai.behaviors.survival import SurvivalBehavior
from uai.behaviors.gather import GatherBehavior
from uai.behaviors.reproduce import ReproduceBehavior
from uai.behaviors.incantation import IncantationBehavior
from uai.behaviors.rally import RallyBehavior
from uai.behaviors.follow import FollowBehavior


class UtilityAIController:
    """
    A Utility-based AI Controller for the Zappy drone.
    Clean refactored architecture using strategy pattern for behaviors.
    """

    def __init__(self, context: DroneContext):
        self.context = context
        self.state = UAIState()
        self.calculator = UtilityCalculator(context, self.state)

        self.behaviors = {
            "survival": SurvivalBehavior(),
            "gather": GatherBehavior(),
            "reproduce": ReproduceBehavior(),
            "incantation": IncantationBehavior(),
            "rally": RallyBehavior(),
            "follow": FollowBehavior(),
        }

        swarm_cfg = get_swarm_config()
        self.state.tick_since_bcast = swarm_cfg.get("BCAST_INTERVAL", 2)

    def _process_messages(self) -> None:
        if self.context.level > self.state.last_level:
            self.state.reset_leader_state()
            self.state.reset_follower_state()
            self.state.last_level = self.context.level

        for bcst in self.context.broadcasts:
            decoded = bcst.content
            if (
                not decoded
                or decoded.team_name != self.context.team_name
                or decoded.level != self.context.level
            ):
                continue

            if self.state.is_following:
                if (
                    decoded.msg_type == MessageType.ABORT
                    and decoded.drone_id == self.state.target_leader_id
                ):
                    self.state.reset_follower_state()
                elif (
                    decoded.msg_type == MessageType.INCANT
                    and decoded.drone_id == self.state.target_leader_id
                ):
                    if bcst.direction in BROADCAST_DIRECTION_ARRIVED:
                        self.state.waiting_incant = True
                    else:
                        self.state.reset_follower_state()

            if self.state.is_leader:
                if (
                    decoded.msg_type == MessageType.READY
                    and bcst.direction in BROADCAST_DIRECTION_ARRIVED
                ):
                    self.state.ready_count += 1
                    self.state.rally_ticks = 0
                elif (
                    decoded.msg_type == MessageType.LEAVING
                    and self.state.ready_count > 0
                ):
                    self.state.ready_count -= 1
                elif (
                    decoded.msg_type == MessageType.RALLY
                    and decoded.drone_id > self.context.drone_id
                ):
                    self.state.reset_leader_state()
                    self.state.is_following = True

            if decoded.msg_type == MessageType.RALLY:
                if decoded.drone_id >= self.state.target_leader_id:
                    self.state.target_leader_id = decoded.drone_id
                    self.state.highest_rally_direction = bcst.direction

    def tick(self) -> str | None:
        if self.context.ticks_since_inventory >= 15:
            self.context.ticks_since_inventory = 0
            return "Inventory"

        self.context.ticks_since_inventory += 1
        self.state.highest_rally_direction = None
        self._process_messages()

        evo_cfg = get_evolution_config()
        if self.context.level >= evo_cfg.get("MAX_LEVEL", 8):
            return self.behaviors["survival"].get_action(self.context, self.state)

        if self.state.leader_aborted:
            self.state.reset_leader_state()
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.ABORT,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if self.state.follower_leaving:
            self.state.reset_follower_state()
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.LEAVING,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if self.state.waiting_incant:
            return None

        repr_cfg = get_reproduction_config()
        if self.context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            self.state.reproduce_connect_sent = False
            self.state.reproduce_fork_sent = False
            self.state.reproduce_spawn_sent = False

        utilities = self.calculator.calculate_all()
        best_behavior = max(utilities.items(), key=lambda item: item[1])[0]

        if utilities[best_behavior] <= 0.0:
            best_behavior = "survival"

        if best_behavior != self.state.last_behavior:
            self.context.path_queue.clear()
            self.state.last_behavior = best_behavior

        if best_behavior == "survival":
            if self.state.is_leader:
                self.state.leader_aborted = True
                return self.tick()
            if self.state.is_following and self.state.ready_sent:
                self.state.follower_leaving = True
                return self.tick()

        if best_behavior == "rally":
            self.state.is_leader = True
            self.state.is_following = False
            self.state.rally_ticks += 1
            swarm_cfg = get_swarm_config()
            if self.state.rally_ticks > swarm_cfg.get("RALLY_TIMEOUT", 100):
                self.state.rally_ticks = 0
                best_behavior = "reproduce"
        elif best_behavior == "follow":
            self.state.is_following = True
            self.state.is_leader = False
        else:
            self.state.rally_ticks = 0

        action = self.behaviors[best_behavior].get_action(self.context, self.state)

        ai_logger.log_state(
            best_behavior,
            action or "None",
            self.context.level,
            self.context.inventory,
        )
        return action
