##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Survival Layer (Highest Priority)
##

import random
from states.AStates import State
from context import DroneContext
from config import FOOD_TARGET, EXPLORE_TURN_EVERY


class ForageFood(State):
    """
    Survival state — Priority 1: Keep the food buffer above the safety threshold.
    """

    def enter(self, context: DroneContext) -> None:
        print("[ForageFood] Entering state.")
        self._forward_streak = 0

    def update(self, context: DroneContext) -> str | None:
        if context.inventory.food >= FOOD_TARGET:
            print("[ForageFood] Buffer secure. Switching to SearchStone.")
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not context.vision:
            return "Look"

        # Food on the current tile — take it immediately.
        if context.vision[0].food > 0:
            self._forward_streak = 0
            return "Take food"

        # No food here — explore.
        # Rotate every 5 steps to avoid getting stuck in a straight line.
        self._forward_streak += 1
        if self._forward_streak % EXPLORE_TURN_EVERY == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def exit(self, context: DroneContext) -> None:
        print("[ForageFood] Exiting state.")
