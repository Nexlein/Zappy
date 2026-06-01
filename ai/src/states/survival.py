##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Survival Layer (Highest Priority)
##

from states.AStates import State
from context import DroneContext


class ForageFood(State):
    """Survival state: Priority 1 is keeping the food buffer high."""

    def enter(self, context: DroneContext) -> str | None:
        print("[ForageFood] Entering state. Scanning environment.")
        return "Look"

    def update(self, context: DroneContext) -> str | None:
        if context.inventory.food >= 15:
            print("[ForageFood] Buffer secure.")
            return None

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if context.vision and len(context.vision) > 0:
            if context.vision[0].food > 0:
                context.vision = []
                return "Take food"

        if not context.vision:
            return "Look"

        context.vision = []
        return "Forward"

    def exit(self, context: DroneContext) -> str | None:
        print("[ForageFood] Exiting state.")
        return None


# The Survival Layer (Highest Priority)
# If the drone runs out of food, it dies and the socket closes.
# You cannot win if your drones are dead. Therefore, any state that prevents the drone from dying belongs in this category.

# Flee or Evade: (A state you might add later) If an enemy team keeps using the Eject command to push your drone away from resources, your drone might need a survival state to run away and regroup.
