##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Survival Layer (Highest Priority)
##

import random
from fsm.states.AState import AState
from context import DroneContext
from utils.config_loader import get_survival_config, get_evolution_config
from protocol.BroadcastProtocol import MessageType
from protocol.look_parser import find_closest_resource_path


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

        # Check if we have a safe amount of food and someone is calling for help
        safe_food = surv_cfg.get("SAFE_FOOD_THRESHOLD", 10)
        if context.inventory.food >= safe_food:
            evo_cfg = get_evolution_config()
            if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
                for bcst in context.broadcasts:
                    if (
                        bcst.content.msg_type == MessageType.RALLY
                        and bcst.content.level == context.level
                    ):
                        return "MapsToAlly"

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

        best_path = find_closest_resource_path(context.vision, ["food"])

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
