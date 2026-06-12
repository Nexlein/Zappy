##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Swarm Layer (Contextual Priority)
##

from states.AStates import State
from context import DroneContext
from elevations import (
    is_incantation_ready,
    ELEVATION_REQUIREMENTS,
    BROADCAST_DIRECTION_ARRIVED,
    BROADCAST_DIRECTION_FORWARD,
    BROADCAST_DIRECTION_RIGHT,
    BROADCAST_DIRECTION_LEFT,
)
from config import SURVIVAL_THRESHOLD, RALLY_TIMEOUT, BCAST_INTERVAL
from BroadcastProtocol import BroadcastProtocol, MessageType
from ai_logger import ai_logger


# --- Helpers ---
def _next_stone_to_drop(context: DroneContext) -> str | None:
    """
    Return the name of the first stone the drone should place on its current
    tile to meet the elevation requirements, or None if nothing more is needed
    or if vision is unavailable.
    """
    if not context.vision:  # guard: vision may be empty
        return None
    tile = context.vision[0]
    for stone, required in ELEVATION_REQUIREMENTS.get(context.level, {}).items():
        if (
            getattr(tile, stone, 0) < required
            and getattr(context.inventory, stone, 0) > 0
        ):
            return stone
    return None


# BroadcastHelp: Yell across the map and wait for allies to arrive.
class BroadcastHelp(State):
    """
    Swarm state — Contextual Priority:
      1. Drop required stones on the current tile.
      2. Broadcast RALLY to call teammates.
      3. Trigger the incantation once the tile is ready.
    """

    def enter(self, context: DroneContext) -> None:
        self.ticks_waited = 0
        self.tick_since_bcast = BCAST_INTERVAL  # broadcast on the first tick
        ai_logger.talk(
            "[BroadcastHelp] Help! I need my teammates to gather here! RALLY!"
        )

    def update(self, context: DroneContext) -> str | None:
        self.ticks_waited += 1

        # Check if the tile is ready for incantation (independent of elif chain).
        if context.vision:
            tile = context.vision[0]
            if is_incantation_ready(context.level, tile):
                return "Incantation"

        # Safety exits (checked regardless of vision state).
        if context.inventory.food < SURVIVAL_THRESHOLD:
            ai_logger.talk(
                "[BroadcastHelp] Waiting is making me hungry! I'm going to look for food."
            )
            return "ForageFood"
        if self.ticks_waited > RALLY_TIMEOUT:
            ai_logger.talk(
                "[BroadcastHelp] Nobody is coming... I will go back to looking for stones."
            )
            return "SearchStone"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not context.vision:
            return "Look"

        # Drop any stone still missing from the tile.
        stone = _next_stone_to_drop(context)
        if stone:
            return f"Set {stone}"

        # Periodically re-broadcast the RALLY signal.
        if self.tick_since_bcast >= BCAST_INTERVAL:
            self.tick_since_bcast = 0
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.RALLY, context.level
            )
            return f"Broadcast {payload}"

        self.tick_since_bcast += 1
        # Re-scan so update() can check is_incantation_ready() with fresh data.
        return "Look"

    def exit(self, context: DroneContext) -> None:
        ai_logger.talk("[BroadcastHelp] Stopping my broadcast.")


# MapsToAlly: Navigate toward a teammate's broadcast signal.
class MapsToAlly(State):
    """
    Swarm state — Contextual Priority:
      Navigate toward the ally broadcasting RALLY by reading the direction K
      from each incoming broadcast message.
    """

    def enter(self, context: DroneContext) -> None:
        ai_logger.talk("[MapsToAlly] I hear someone! I'm on my way to help!")
        self._entry_level = context.level
        self.ticks_waited = 0
        self.arrived = False

    def update(self, context: DroneContext) -> str | None:
        self.ticks_waited += 1

        if context.inventory.food < SURVIVAL_THRESHOLD:
            ai_logger.talk("[MapsToAlly] I'm too hungry to keep walking... Food first!")
            return "ForageFood"

        # Another player's incantation already leveled us up.
        if context.level > self._entry_level:
            ai_logger.talk(
                "[MapsToAlly] Wow, I leveled up on the way thanks to someone else!"
            )
            return "SearchStone"

        if self.ticks_waited > RALLY_TIMEOUT:
            ai_logger.talk(
                "[MapsToAlly] I lost the signal... back to searching myself."
            )
            return "SearchStone"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        # -- Already on the rally tile: drop stones and wait. --
        if self.arrived:
            if not context.vision:
                return "Look"
            stone = _next_stone_to_drop(context)
            if stone:
                return f"Set {stone}"
            return "Look"  # Keep re-scanning so update() sees is_incantation_ready

        # -- Still travelling: follow the latest RALLY broadcast direction. --
        for bcst in context.broadcasts:
            if (
                bcst.content.msg_type == MessageType.RALLY
                and bcst.content.level == self._entry_level
            ):
                direction = bcst.direction

                if direction in BROADCAST_DIRECTION_ARRIVED:
                    self.arrived = True
                    return "Look"

                # Ahead or slightly diagonal ahead.
                if direction in BROADCAST_DIRECTION_FORWARD:
                    return "Forward"

                # To the right, behind-right, or directly behind.
                if direction in BROADCAST_DIRECTION_RIGHT:
                    return "Right"

                # To the left or behind-left.
                if direction in BROADCAST_DIRECTION_LEFT:
                    return "Left"

        # No RALLY heard this tick — stay put and wait for the next broadcast.
        return "Look"

    def exit(self, context: DroneContext) -> None:
        ai_logger.talk("[MapsToAlly] I am leaving the group-up journey.")
