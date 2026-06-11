##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Evolution Layer (Medium Priority)
##

import random
from states.AStates import State
from context import DroneContext
from elevations import ELEVATION_REQUIREMENTS
from BroadcastProtocol import MessageType
from config import (
    MAX_LEVEL,
    SOLO_INCANTATION_LEVEL,
    SURVIVAL_THRESHOLD,
    EXPLORE_TURN_EVERY,
)
from look_parser import generate_path_to_tile


class SearchStone(State):
    """
    Evolution state — Priority 2: Gather the stones required to level up.

    Vision contract:
      - Does NOT clear context.vision manually.
      - After 'Take <stone>' the main loop decrements vision[0].<stone> in place,
        so further takes on the same tile work without an extra Look.
    """

    def enter(self, context: DroneContext) -> None:
        print(f"[SearchStone] Hunting for stones to reach level {context.level + 1}.")
        self._forward_streak = 0

    def _get_missing_stones(self, context: DroneContext) -> dict[str, int]:
        """Return stones still needed compared to the current inventory."""
        requirements = ELEVATION_REQUIREMENTS.get(context.level, {})
        return {
            stone: required - getattr(context.inventory, stone, 0)
            for stone, required in requirements.items()
            if getattr(context.inventory, stone, 0) < required
        }

    def update(self, context: DroneContext) -> str | None:
        if context.level >= MAX_LEVEL:
            print("[SearchStone] Already at maximum level. Switching to ForageFood.")
            return "ForageFood"

        if context.inventory.food < SURVIVAL_THRESHOLD:
            print("[SearchStone] Low food! Switching to ForageFood.")
            return "ForageFood"

        # React to a teammate's RALLY call (solo levels can incant alone).
        if context.level > SOLO_INCANTATION_LEVEL:
            for bcst in context.broadcasts:
                if (
                    bcst.content.msg_type == MessageType.RALLY
                    and bcst.content.level == context.level
                ):
                    print(
                        f"[SearchStone] Heard RALLY for level {context.level}. "
                        "Switching to MapsToAlly."
                    )
                    return "MapsToAlly"

        missing = self._get_missing_stones(context)
        if not missing:
            print("[SearchStone] All required stones collected!")
            # Solo levels can incant alone; higher levels need teammates.
            return (
                "Incantation"
                if context.level <= SOLO_INCANTATION_LEVEL
                else "BroadcastHelp"
            )

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if context.path_queue:
            return context.path_queue.pop(0)

        if not context.vision:
            return "Look"

        missing_stones = self._get_missing_stones(context)
        current_tile = context.vision[0]

        # Pick up any needed stone on the current tile.
        for stone in missing_stones:
            if getattr(current_tile, stone, 0) > 0:
                self._forward_streak = 0
                return f"Take {stone}"

        # Check vision for needed stones ahead
        for i, tile in enumerate(context.vision):
            if i == 0:
                continue
            for stone in missing_stones:
                if getattr(tile, stone, 0) > 0:
                    self._forward_streak = 0
                    path = generate_path_to_tile(i)
                    if path:
                        context.path_queue.extend(path)
                        return context.path_queue.pop(0)

        # Nothing useful here — explore.
        # Rotate every 5 steps to avoid getting stuck in a straight line.
        self._forward_streak += 1
        if self._forward_streak % EXPLORE_TURN_EVERY == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def exit(self, context: DroneContext) -> None:
        print("[SearchStone] Mining phase finished.")


class IncantationState(State):
    """
    Evolution state: Execute the incantation ritual.
    """

    def enter(self, context: DroneContext) -> None:
        print("[Incantation] Initiating elevation ritual…")
        self.command_sent = False

    def update(self, context: DroneContext) -> str | None:
        if self.command_sent and context.last_command_successful is not None:
            if context.last_command_successful:
                print(f"[Incantation] Elevation succeeded! Now level {context.level}.")
            else:
                print("[Incantation] Elevation failed. Regrouping.")
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not self.command_sent:
            self.command_sent = True
            return "Incantation"
        return None

    def exit(self, context: DroneContext) -> None:
        pass
