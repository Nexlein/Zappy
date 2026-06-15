import random
from context import DroneContext
from elevations import (
    ELEVATION_REQUIREMENTS,
    PLAYERS_REQUIRED,
    is_incantation_ready,
    BROADCAST_DIRECTION_ARRIVED,
    BROADCAST_DIRECTION_FORWARD,
    BROADCAST_DIRECTION_RIGHT,
    BROADCAST_DIRECTION_LEFT,
)
from config import (
    FOOD_TARGET,
    SURVIVAL_THRESHOLD,
    BCAST_INTERVAL,
    MAX_LEVEL,
    SOLO_INCANTATION_LEVEL,
)
from BroadcastProtocol import BroadcastProtocol, MessageType
from look_parser import generate_path_to_tile
from ai_logger import ai_logger


class UtilityAIController:
    """
    A Utility-based AI Controller for the Zappy drone, fully synced with the FSM
    robustness features (drone_id tie-breaker, READY/LEAVING/INCANT/ABORT handshakes).
    """

    def __init__(self, context: DroneContext):
        self.context = context
        self._forward_streak = 0
        self.tick_since_bcast = BCAST_INTERVAL

        # Leader state
        self.ready_count = 0
        self.is_leader = False
        self.leader_aborted = False

        # Incantation sequence state
        self.incant_bcast_sent = False
        self.incant_cmd_sent = False

        # Follower state
        self.is_following = False
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self.follower_leaving = False

        self.last_level = context.level

    def _reset_leader_state(self):
        self.ready_count = 0
        self.is_leader = False
        self.leader_aborted = False
        self.incant_bcast_sent = False
        self.incant_cmd_sent = False

    def _reset_follower_state(self):
        self.is_following = False
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self.follower_leaving = False

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
                if decoded.msg_type == MessageType.ABORT:
                    ai_logger.talk("[UAI-Follower] Rally aborted. Resetting.")
                    self._reset_follower_state()
                elif decoded.msg_type == MessageType.INCANT:
                    if bcst.direction in BROADCAST_DIRECTION_ARRIVED:
                        ai_logger.talk("[UAI-Follower] Ritual starting! Freezing.")
                        self.waiting_incant = True
                    else:
                        ai_logger.talk("[UAI-Follower] Ritual started without me.")
                        self._reset_follower_state()

            # Leader listening to Followers / Other Leaders
            if self.is_leader:
                if (
                    decoded.msg_type == MessageType.READY
                    and bcst.direction in BROADCAST_DIRECTION_ARRIVED
                ):
                    self.ready_count += 1
                    ai_logger.talk(
                        f"[UAI-Leader] Teammate ready! ({self.ready_count + 1}/{PLAYERS_REQUIRED[self.context.level]})"
                    )
                elif decoded.msg_type == MessageType.LEAVING and self.ready_count > 0:
                    self.ready_count -= 1
                    ai_logger.talk(
                        f"[UAI-Leader] Teammate left... ({self.ready_count + 1}/{PLAYERS_REQUIRED[self.context.level]})"
                    )
                elif (
                    decoded.msg_type == MessageType.RALLY
                    and decoded.drone_id > self.context.drone_id
                ):
                    ai_logger.talk(
                        f"[UAI-Leader] Yielding leadership to {decoded.drone_id[:4]}."
                    )
                    self._reset_leader_state()
                    self.is_following = True

    def tick(self) -> str | None:
        if self.context.ticks_since_inventory >= 15:
            self.context.ticks_since_inventory = 0
            return "Inventory"

        self.context.ticks_since_inventory += 1

        self._process_messages()

        if self.context.level >= MAX_LEVEL:
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

        # Compute Utilities
        u_survival = self._get_survival_utility()
        u_incantation = self._get_incantation_utility()
        u_rally = self._get_rally_utility(u_survival)
        u_follow = self._get_follow_utility(u_survival)
        u_gather = self._get_gather_utility(u_survival)

        utilities = {
            "survival": u_survival,
            "incantation": u_incantation,
            "rally": u_rally,
            "follow": u_follow,
            "gather": u_gather,
        }

        best_behavior = max(utilities.items(), key=lambda item: item[1])[0]

        if utilities[best_behavior] <= 0.0:
            best_behavior = "survival"

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
        elif best_behavior == "follow":
            self.is_following = True
            self.is_leader = False

        # Execute Action
        if best_behavior == "incantation":
            action = self._get_incantation_action()
        elif best_behavior == "rally":
            action = self._get_rally_action()
        elif best_behavior == "follow":
            action = self._get_follow_action()
        elif best_behavior == "gather":
            action = self._get_gather_action()
        else:
            action = self._get_survival_action()

        ai_logger.log_state(best_behavior, action or "None")
        return action

    # --- Utility Calculations ---

    def _get_survival_utility(self) -> float:
        food = self.context.inventory.food
        if food < SURVIVAL_THRESHOLD:
            return 1.0
        if food >= FOOD_TARGET:
            return 0.0
        return 1.0 - ((food - SURVIVAL_THRESHOLD) / (FOOD_TARGET - SURVIVAL_THRESHOLD))

    def _get_incantation_utility(self) -> float:
        if not self.context.vision:
            return 0.0
        tile = self.context.vision[0]

        # If we already sent the incantation command, stick to this state until success/fail
        if self.incant_cmd_sent:
            return 1.0

        if self.context.level <= SOLO_INCANTATION_LEVEL:
            if is_incantation_ready(self.context.level, tile):
                return 1.0
        else:
            if (
                self.is_leader
                and self.ready_count + 1 >= PLAYERS_REQUIRED.get(self.context.level, 0)
                and is_incantation_ready(self.context.level, tile)
            ):
                return 1.0
        return 0.0

    def _get_gather_utility(self, u_survival: float) -> float:
        missing = self._get_missing_stones()
        if not missing:
            return 0.0
        num_missing = sum(missing.values())
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        total_needed = sum(requirements.values()) if requirements else 1

        stone_ratio = num_missing / total_needed
        return 0.8 * stone_ratio * (1.0 - u_survival)

    def _get_rally_utility(self, u_survival: float) -> float:
        if self.context.level <= SOLO_INCANTATION_LEVEL:
            return 0.0
        if self._get_missing_stones():
            return 0.0

        # If we heard a rally from someone else and yielded, we don't rally
        if self.is_following:
            return 0.0

        return 0.9 * (1.0 - u_survival)

    def _get_follow_utility(self, u_survival: float) -> float:
        if self.context.level <= SOLO_INCANTATION_LEVEL:
            return 0.0

        if self.is_following:
            return 0.85 * (1.0 - u_survival)

        missing = self._get_missing_stones()
        if not missing:
            # We have all stones, but if someone with a higher ID called, we follow them!
            for bcst in self.context.broadcasts:
                decoded = bcst.content
                if (
                    decoded
                    and decoded.team_name == self.context.team_name
                    and decoded.msg_type == MessageType.RALLY
                    and decoded.level == self.context.level
                ):
                    if decoded.drone_id > self.context.drone_id:
                        return 0.85 * (1.0 - u_survival)
            return 0.0

        for bcst in self.context.broadcasts:
            decoded = bcst.content
            if (
                decoded
                and decoded.team_name == self.context.team_name
                and decoded.msg_type == MessageType.RALLY
                and decoded.level == self.context.level
            ):
                return 0.85 * (1.0 - u_survival)
        return 0.0

    # --- Action Generation ---

    def _get_missing_stones(self) -> dict[str, int]:
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        return {
            stone: required - getattr(self.context.inventory, stone, 0)
            for stone, required in requirements.items()
            if getattr(self.context.inventory, stone, 0) < required
        }

    def _next_stone_to_drop(self) -> str | None:
        if not self.context.vision:
            return None
        tile = self.context.vision[0]
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        for stone, required in requirements.items():
            if (
                getattr(tile, stone, 0) < required
                and getattr(self.context.inventory, stone, 0) > 0
            ):
                return stone
        return None

    def _get_survival_action(self) -> str:
        if self.context.path_queue:
            return self.context.path_queue.pop(0)
        if not self.context.vision:
            return "Look"
        if self.context.vision[0].food > 0:
            self._forward_streak = 0
            return "Take food"
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            if tile.food > 0:
                self._forward_streak = 0
                path = generate_path_to_tile(i)
                if path:
                    self.context.path_queue.extend(path)
                    return self.context.path_queue.pop(0)
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_gather_action(self) -> str:
        if self.context.path_queue:
            return self.context.path_queue.pop(0)
        if not self.context.vision:
            return "Look"
        missing = self._get_missing_stones()
        current_tile = self.context.vision[0]
        for stone in missing:
            if getattr(current_tile, stone, 0) > 0:
                self._forward_streak = 0
                return f"Take {stone}"
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            for stone in missing:
                if getattr(tile, stone, 0) > 0:
                    self._forward_streak = 0
                    path = generate_path_to_tile(i)
                    if path:
                        self.context.path_queue.extend(path)
                        return self.context.path_queue.pop(0)
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_incantation_action(self) -> str | None:
        # Check if previous incantation failed
        if self.incant_cmd_sent and self.context.last_command_successful is False:
            self.leader_aborted = True
            return self.tick()  # trigger abort

        if not self.incant_bcast_sent and self.context.level > SOLO_INCANTATION_LEVEL:
            self.incant_bcast_sent = True
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.INCANT,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if not self.incant_cmd_sent:
            self.incant_cmd_sent = True
            return "Incantation"
        return None

    def _get_rally_action(self) -> str:
        if not self.context.vision:
            return "Look"
        stone = self._next_stone_to_drop()
        if stone:
            return f"Set {stone}"
        if self.tick_since_bcast >= BCAST_INTERVAL:
            self.tick_since_bcast = 0
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.RALLY,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"
        self.tick_since_bcast += 1
        return "Look"

    def _get_follow_action(self) -> str:
        rally_msg = None
        for bcst in reversed(self.context.broadcasts):
            decoded = bcst.content
            if (
                decoded
                and decoded.team_name == self.context.team_name
                and decoded.msg_type == MessageType.RALLY
                and decoded.level == self.context.level
            ):
                rally_msg = bcst
                break

        if not rally_msg:
            return "Look"

        direction = rally_msg.direction
        if direction in BROADCAST_DIRECTION_ARRIVED:
            self.arrived = True
            if not self.context.vision:
                return "Look"
            stone = self._next_stone_to_drop()
            if stone:
                return f"Set {stone}"
            if not self.ready_sent:
                self.ready_sent = True
                payload = BroadcastProtocol.encode(
                    self.context.team_name,
                    MessageType.READY,
                    self.context.level,
                    self.context.drone_id,
                )
                return f"Broadcast {payload}"
            return "Look"

        if direction in BROADCAST_DIRECTION_FORWARD:
            return "Forward"
        if direction in BROADCAST_DIRECTION_RIGHT:
            return "Right"
        if direction in BROADCAST_DIRECTION_LEFT:
            return "Left"
        return "Look"
