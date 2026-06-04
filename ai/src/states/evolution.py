##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Evolution Layer (Medium Priority)
##

from states.AStates import State
from context import DroneContext
from elevations import ELEVATION_REQUIREMENTS
from BroadcastProtocol import BroadcastProtocol, MessageType


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

        if context.level > 1:
            for bcst in context.broadcasts:
                try:
                    decoded = BroadcastProtocol.decode(bcst.text)
                    if (
                        decoded
                        and decoded.team_name == context.team_name
                        and decoded.msg_type == MessageType.RALLY
                        and decoded.level == context.level
                    ):
                        print(
                            f"[SearchStone] Heard RALLY from teammate of level {context.level}. Transitioning to MapsToAlly."
                        )
                        return "MapsToAlly"
                except ValueError:
                    continue

        missing = self._get_missing_stones(context)
        if not missing:
            print("[SearchStone] All required stones collected!")
            return "Incantation" if context.level == 1 else "BroadcastHelp"

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


class IncantationState(State):
    """Evolution state: Handles the actual incantation ritual."""

    def enter(self, context: DroneContext) -> str | None:
        print("[Incantation] Initiating elevation ritual...")
        self.initiated = False
        return None

    def update(self, context: DroneContext) -> str | None:
        if self.initiated:
            # We got the result (since sent_commands became empty)
            if context.last_command_successful:
                print(f"[Incantation] Elevation succeeded! Now level {context.level}.")
            else:
                print("[Incantation] Elevation failed.")
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not self.initiated:
            self.initiated = True
            return "Incantation"
        return None

    def exit(self, context: DroneContext) -> str | None:
        return None
