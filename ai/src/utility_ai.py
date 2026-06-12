##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Utility-based AI Controller
##

import random
from context import DroneContext
from elevations import (
    ELEVATION_REQUIREMENTS,
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
    A Utility-based AI Controller for the Zappy drone.
    Calculates utility scores for different behaviors every tick and
    executes the action associated with the highest utility.
    """

    def __init__(self, context: DroneContext):
        self.context = context
        self._forward_streak = 0
        self.tick_since_bcast = BCAST_INTERVAL

    def tick(self) -> str | None:
        """Evaluate utilities of all behaviors and return the best action."""
        if self.context.ticks_since_inventory >= 15:
            self.context.ticks_since_inventory = 0
            return "Inventory"

        self.context.ticks_since_inventory += 1

        if self.context.level >= MAX_LEVEL:
            # At max level, just survive
            return self._get_survival_action()

        # 1. Compute Utilities
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

        # Select behavior with maximum utility
        best_behavior = max(utilities.items(), key=lambda item: item[1])[0]

        # If all utilities are 0, default to survival
        if utilities[best_behavior] <= 0.0:
            best_behavior = "survival"

        # 2. Return the action for the chosen behavior
        action = "survival"
        if best_behavior == "incantation":
            action = "Incantation"
        elif best_behavior == "rally":
            action = self._get_rally_action()
        elif best_behavior == "follow":
            action = self._get_follow_action()
        elif best_behavior == "gather":
            action = self._get_gather_action()
        else:
            action = self._get_survival_action()

        ai_logger.log_state(best_behavior, action)
        return action

    # --- Utility Calculations ---

    def _get_survival_utility(self) -> float:
        """Calculate survival utility between 0.0 and 1.0."""
        food = self.context.inventory.food
        if food < SURVIVAL_THRESHOLD:
            return 1.0
        if food >= FOOD_TARGET:
            return 0.0
        # Linear transition from SURVIVAL_THRESHOLD to FOOD_TARGET
        return 1.0 - ((food - SURVIVAL_THRESHOLD) / (FOOD_TARGET - SURVIVAL_THRESHOLD))

    def _get_incantation_utility(self) -> float:
        """If incantation is ready on current tile, utility is absolute."""
        if not self.context.vision:
            return 0.0
        tile = self.context.vision[0]
        if is_incantation_ready(self.context.level, tile):
            return 1.0
        return 0.0

    def _get_gather_utility(self, u_survival: float) -> float:
        """Utility of gathering missing stones for the level."""
        missing = self._get_missing_stones()
        if not missing:
            return 0.0
        # Higher utility if we have plenty of food and many stones missing
        num_missing = sum(missing.values())
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        total_needed = sum(requirements.values()) if requirements else 1

        stone_ratio = num_missing / total_needed
        return 0.8 * stone_ratio * (1.0 - u_survival)

    def _get_rally_utility(self, u_survival: float) -> float:
        """Utility of calling teammates when ready for incantation."""
        if self.context.level <= SOLO_INCANTATION_LEVEL:
            return 0.0  # Solo levels don't need to rally

        # We need all stones collected to rally
        missing = self._get_missing_stones()
        if missing:
            return 0.0

        # High priority to rally if we are safe
        return 0.9 * (1.0 - u_survival)

    def _get_follow_utility(self, u_survival: float) -> float:
        """Utility of following a teammate's rally."""
        if self.context.level <= SOLO_INCANTATION_LEVEL:
            return 0.0  # Can level up alone, no need to group up

        # Only follow if we still need to level up and don't have our own rally ready
        missing = self._get_missing_stones()
        if not missing:
            return 0.0  # We have all stones, we should be the one rallying

        # Check if we heard a rally from our team at our level
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
        """Get dict of missing stones for current level elevation."""
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        return {
            stone: required - getattr(self.context.inventory, stone, 0)
            for stone, required in requirements.items()
            if getattr(self.context.inventory, stone, 0) < required
        }

    def _next_stone_to_drop(self) -> str | None:
        """Find the next stone in inventory to place on tile 0."""
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
        """Action logic for finding food."""
        if self.context.path_queue:
            return self.context.path_queue.pop(0)

        if not self.context.vision:
            return "Look"

        if self.context.vision[0].food > 0:
            self._forward_streak = 0
            return "Take food"

        # Check vision for food ahead
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            if tile.food > 0:
                self._forward_streak = 0
                path = generate_path_to_tile(i)
                if path:
                    self.context.path_queue.extend(path)
                    return self.context.path_queue.pop(0)

        # Explore forward or turn
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_gather_action(self) -> str:
        """Action logic for mining missing stones."""
        if self.context.path_queue:
            return self.context.path_queue.pop(0)

        if not self.context.vision:
            return "Look"

        missing = self._get_missing_stones()
        current_tile = self.context.vision[0]

        # Take any needed stone from the current tile
        for stone in missing:
            if getattr(current_tile, stone, 0) > 0:
                self._forward_streak = 0
                return f"Take {stone}"

        # Check vision for needed stones ahead
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

        # Walk forward to find more
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_rally_action(self) -> str:
        """Action logic for summoning allies to incantation."""
        if not self.context.vision:
            return "Look"

        # Drop stones we have to prepare the tile
        stone = self._next_stone_to_drop()
        if stone:
            return f"Set {stone}"

        # Broadcast rally
        if self.tick_since_bcast >= BCAST_INTERVAL:
            self.tick_since_bcast = 0
            payload = BroadcastProtocol.encode(
                self.context.team_name, MessageType.RALLY, self.context.level
            )
            return f"Broadcast {payload}"

        self.tick_since_bcast += 1
        return "Look"

    def _get_follow_action(self) -> str:
        """Action logic for walking to teammate's rally."""
        # Find the latest rally message
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
            # We are on the rally tile, drop stones to help
            if not self.context.vision:
                return "Look"
            stone = self._next_stone_to_drop()
            if stone:
                return f"Set {stone}"
            return "Look"

        if direction in BROADCAST_DIRECTION_FORWARD:
            return "Forward"
        if direction in BROADCAST_DIRECTION_RIGHT:
            return "Right"
        if direction in BROADCAST_DIRECTION_LEFT:
            return "Left"

        return "Look"
