##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Evolution Layer (Medium Priority)
##

import random
from fsm.states.AState import State
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
from BroadcastProtocol import BroadcastProtocol


class SearchStone(State):
    """
    Evolution state — Priority 2: Gather the stones required to level up.

    Vision contract:
      - Does NOT clear context.vision manually.
      - After 'Take <stone>' the main loop decrements vision[0].<stone> in place,
        so further takes on the same tile work without an extra Look.
    """

    def enter(self, context: DroneContext) -> None:
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
            return "ForageFood"

        if context.inventory.food < SURVIVAL_THRESHOLD:
            return "ForageFood"

        # React to a teammate's RALLY call (solo levels can incant alone).
        if context.level > SOLO_INCANTATION_LEVEL:
            for bcst in context.broadcasts:
                if (
                    bcst.content.msg_type == MessageType.RALLY
                    and bcst.content.level == context.level
                ):
                    return "MapsToAlly"

        missing = self._get_missing_stones(context)
        if not missing:
            return "BroadcastHelp"

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
        best_path = None
        for i, tile in enumerate(context.vision):
            if i == 0:
                continue
            for stone in missing_stones:
                if getattr(tile, stone, 0) > 0:
                    path = generate_path_to_tile(i)
                    if best_path is None or len(path) < len(best_path):
                        best_path = path
                    break

        if best_path:
            self._forward_streak = 0
            context.path_queue.extend(best_path)
            return context.path_queue.pop(0)

        # Nothing useful here — explore.
        # Rotate every 5 steps to avoid getting stuck in a straight line.
        self._forward_streak += 1
        if self._forward_streak % EXPLORE_TURN_EVERY == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def exit(self, context: DroneContext) -> None:
        pass


class IncantationState(State):
    """
    Evolution state: Execute the incantation ritual.
    """

    def enter(self, context: DroneContext) -> None:
        self.broadcast_sent = False
        self.command_sent = False
        self.need_abort = False
        self.abort_sent = False

    def update(self, context: DroneContext) -> str | None:
        """
        Read the ritual verdict; on a failed group ritual, get_action emits
        ABORT before the state exits to SearchStone.
        """
        if self.abort_sent:
            return "SearchStone"
        if self.need_abort:
            return None

        if self.command_sent and context.last_command_successful is not None:
            if context.last_command_successful:
                return "SearchStone"
            if context.level > SOLO_INCANTATION_LEVEL:
                self.need_abort = True
                return None
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        """
        Group levels: Broadcast INCANT one tick before Incantation
        (or ABORT if the ritual just failed). Solo levels incant directly.
        """
        if self.need_abort and not self.abort_sent:
            self.abort_sent = True
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.ABORT, context.level, context.drone_id
            )
            return f"Broadcast {payload}"
        if not self.broadcast_sent and context.level > SOLO_INCANTATION_LEVEL:
            self.broadcast_sent = True
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.INCANT, context.level, context.drone_id
            )
            return f"Broadcast {payload}"
        if not self.command_sent:
            self.command_sent = True
            return "Incantation"
        return None

    def exit(self, context: DroneContext) -> None:
        pass
