import random
from utils.stones import get_missing_stones
from utils.config_loader import get_survival_config, get_evolution_config
from protocol.look_parser import find_closest_resource_path
from uai.behaviors.ABehavior import ABehavior
from context import DroneContext
from uai.state import UAIState


class GatherBehavior(ABehavior):
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        if context.path_queue:
            return context.path_queue.pop(0)
        if not context.vision:
            return "Look"
        missing = get_missing_stones(context.level, context.inventory)
        current_tile = context.vision[0]

        if current_tile.player <= 1:
            for stone in missing:
                if getattr(current_tile, stone, 0) > 0:
                    state.forward_streak = 0
                    return f"Take {stone}"

        surv_cfg = get_survival_config()
        if current_tile.food > 0 and context.inventory.food < surv_cfg.get(
            "FOOD_CEILING", 45
        ):
            state.forward_streak = 0
            return "Take food"

        evo_cfg = get_evolution_config()
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            others = current_tile.player - 1
            swarm_active = any(
                info.is_rallying or info.is_ready or info.is_coming
                for info in context.ally_roster.values()
            )
            if others >= evo_cfg.get("SABOTAGE_GATHER_MIN", 4) and not swarm_active:
                return "Eject"

        best_path = find_closest_resource_path(context.vision, missing)

        if best_path:
            state.forward_streak = 0
            context.path_queue.extend(best_path)
            return context.path_queue.pop(0)
        state.forward_streak += 1
        if state.forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"
