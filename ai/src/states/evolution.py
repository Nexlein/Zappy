##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Evolution Layer (Medium Priority)
##

from states.AStates import State
from context import DroneContext
from elevations import ELEVATION_REQUIREMENTS


class SearchStone(State):
    """Evolution state: Priority 2 - Focuses on gathering stones required to level up."""

    def enter(self, context: DroneContext) -> str | None:
        print(f"[SearchStone] Hunting for stones to reach level {context.level + 1}.")
        return "Look"

    def _get_missing_stones(self, context: DroneContext) -> dict[str, int]:
        """Compares current inventory to the target requirements."""
        requirements = ELEVATION_REQUIREMENTS.get(context.level, {})
        missing = {}

        for stone, required_amount in requirements.items():
            current_amount = getattr(context.inventory, stone, 0)
            if current_amount < required_amount:
                missing[stone] = required_amount - current_amount

        return missing

    def update(self, context: DroneContext) -> str | None:
        if context.inventory.food < 5:
            print("[SearchStone] Emergency! Low food. Switching back to ForageFood.")
            return "ForageFood"

        missing = self._get_missing_stones(context)
        if not missing:
            print("[SearchStone] All required stones collected!")
            return "IncantationState" if context.level == 1 else "BroadcastHelp"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        missing_stones = self._get_missing_stones(context)

        if context.vision and len(context.vision) > 0:
            current_tile = context.vision[0]
            for stone in missing_stones.keys():
                if getattr(current_tile, stone, 0) > 0:
                    context.vision = []
                    return f"Take {stone}"

        if not context.vision:
            return "Look"

        context.vision = []
        return "Forward"

    def exit(self, context: DroneContext) -> str | None:
        print("[SearchStone] Mining phase finished.")
        return None


# The Evolution Layer (Medium Priority)
# Once the drone knows it has enough food to survive the next few hundred ticks, it can focus on the actual objective of the game: leveling up.

# Incantation: The actual act of triggering the ritual
