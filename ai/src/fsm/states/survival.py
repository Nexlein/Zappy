##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Survival Layer (Highest Priority)
##

import random
from fsm.states.AState import AState
from context import DroneContext
from utils.config_loader import get_survival_config


from look_parser import generate_path_to_tile


class ForageFood(AState):
    """
    Survival state — Priority 1: Keep the food buffer above the safety threshold.
    """

    def enter(self, context: DroneContext) -> None:
        self._forward_streak = 0

    def update(self, context: DroneContext) -> str | None:
        surv_cfg = get_survival_config()
        if context.inventory.food >= surv_cfg.get("FOOD_TARGET", 15):
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        if context.path_queue:
            return context.path_queue.pop(0)

        if not context.vision:
            return "Look"

        # Food on the current tile — take it immediately.
        if context.vision[0].food > 0:
            self._forward_streak = 0
            return "Take food"

        # Check vision for food ahead
        best_path = None
        for i, tile in enumerate(context.vision):
            if i == 0:
                continue
            if tile.food > 0:
                path = generate_path_to_tile(i)
                if best_path is None or len(path) < len(best_path):
                    best_path = path

        if best_path:
            self._forward_streak = 0
            context.path_queue.extend(best_path)
            return context.path_queue.pop(0)

        # No food visible — explore.
        # Rotate every 5 steps to avoid getting stuck in a straight line.
        self._forward_streak += 1
        surv_cfg = get_survival_config()
        explore_turn_every = surv_cfg.get("EXPLORE_TURN_EVERY", 5)
        if self._forward_streak % explore_turn_every == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def exit(self, context: DroneContext) -> None:
        pass
