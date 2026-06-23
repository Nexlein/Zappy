##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Survival Layer (Highest Priority)
##

from queen.states.AState import AState
from queen.states.StateNames import AIState
from context import DroneContext
from utils.config_loader import get_survival_config, get_evolution_config
from utils.exploration import get_exploration_action


class ForageFood(AState):
    """
    Survival state — Priority 1: Keep the food buffer above the safety threshold.
    """

    def enter(self, context: DroneContext) -> None:
        self._forward_streak = 0

    def update(self, context: DroneContext) -> str | None:
        surv_cfg = get_survival_config()
        if context.inventory.food >= surv_cfg.get("FOOD_TARGET", 15):
            return AIState.SEARCH_STONE

        # Check if we have a safe amount of food and someone is calling for help
        safe_food = surv_cfg.get("SAFE_FOOD_THRESHOLD", 10)
        if context.inventory.food >= safe_food:
            evo_cfg = get_evolution_config()
            if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
                if any(
                    info.is_rallying and info.level == context.level
                    for info in context.ally_roster.values()
                ):
                    return AIState.MAPS_TO_ALLY

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

        surv_cfg = get_survival_config()
        action, self._forward_streak = get_exploration_action(
            context,
            ["food"],
            surv_cfg.get("EXPLORE_TURN_EVERY", 5),
            self._forward_streak,
        )
        return action

    def exit(self, context: DroneContext) -> None:
        pass
