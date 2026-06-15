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
    PLAYERS_REQUIRED,
    BROADCAST_DIRECTION_ARRIVED,
    BROADCAST_DIRECTION_FORWARD,
    BROADCAST_DIRECTION_RIGHT,
    BROADCAST_DIRECTION_LEFT,
)
from config import (
    SURVIVAL_THRESHOLD,
    RALLY_TIMEOUT,
    BCAST_INTERVAL,
    SOLO_INCANTATION_LEVEL,
)
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
        self.ready_count = 0
        self._abort_target = None
        self._abort_emitted = False
        ai_logger.talk(
            "[BroadcastHelp] Help! I need my teammates to gather here! RALLY!"
        )

    def update(self, context: DroneContext) -> str | None:
        """
        Count READY (K=0) / LEAVING confirmations; launch the incantation once
        enough teammates confirmed AND the tile passes is_incantation_ready.
        On hunger/timeout, get_action emits ABORT before the state exits.
        """
        if self._abort_target:
            return self._abort_target if self._abort_emitted else None

        self.ticks_waited += 1

        for bcst in context.broadcasts:
            if bcst.content.level != context.level:
                continue
            if (
                bcst.content.msg_type == MessageType.READY
                and bcst.direction in BROADCAST_DIRECTION_ARRIVED
            ):
                self.ready_count += 1
                ai_logger.talk(
                    f"[BroadcastHelp] A teammate is ready! "
                    f"({self.ready_count + 1}/{PLAYERS_REQUIRED[context.level]})"
                )
            elif bcst.content.msg_type == MessageType.LEAVING and self.ready_count > 0:
                self.ready_count -= 1
                ai_logger.talk(
                    f"[BroadcastHelp] A teammate left... "
                    f"({self.ready_count + 1}/{PLAYERS_REQUIRED[context.level]})"
                )

        if context.vision:
            tile = context.vision[0]
            if self.ready_count + 1 >= PLAYERS_REQUIRED[
                context.level
            ] and is_incantation_ready(context.level, tile):
                return "Incantation"

        # Yield to another drone with a higher ID if they are calling for the same level
        if context.level > SOLO_INCANTATION_LEVEL:
            for bcst in context.broadcasts:
                if (
                    bcst.content.msg_type == MessageType.RALLY
                    and bcst.content.level == context.level
                    and bcst.content.drone_id > context.drone_id
                ):
                    ai_logger.talk(
                        f"[BroadcastHelp] Yielding to leader {bcst.content.drone_id[:4]}... transitioning to MapsToAlly."
                    )
                    return "MapsToAlly"
        if context.inventory.food < SURVIVAL_THRESHOLD:
            ai_logger.talk(
                "[BroadcastHelp] Waiting is making me hungry! I'm going to look for food."
            )
            self._abort_target = "ForageFood"
        elif self.ticks_waited > RALLY_TIMEOUT:
            ai_logger.talk(
                "[BroadcastHelp] Nobody is coming... I will go back to looking for stones."
            )
            self._abort_target = "SearchStone"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if self._abort_target and not self._abort_emitted:
            self._abort_emitted = True
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.ABORT, context.level, context.drone_id
            )
            return f"Broadcast {payload}"

        if not context.vision:
            return "Look"

        # Drop any stone still missing from the tile.
        stone = _next_stone_to_drop(context)
        if stone:
            return f"Set {stone}"

        # Periodically re-broadcast the RALLY signal (only if higher than solo).
        if context.level > SOLO_INCANTATION_LEVEL:
            if self.tick_since_bcast >= BCAST_INTERVAL:
                self.tick_since_bcast = 0
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    MessageType.RALLY,
                    context.level,
                    context.drone_id,
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
        self.ready_sent = False
        self.waiting_incant = False
        self._leave_target = None
        self._leave_emitted = False

    def _leave(self, target: str) -> str | None:
        """Exit toward `target`, emitting LEAVING first if we said READY."""
        if not self.ready_sent:
            return target
        self._leave_target = target
        return None

    def update(self, context: DroneContext) -> str | None:
        """
        Exit on hunger/level-up/timeout (emitting LEAVING first if READY was
        sent) and react to the initiator's INCANT (K=0: hold still, else too
        late) and ABORT signals.
        """
        if self._leave_target:
            return self._leave_target if self._leave_emitted else None

        self.ticks_waited += 1

        if context.inventory.food < SURVIVAL_THRESHOLD:
            ai_logger.talk("[MapsToAlly] I'm too hungry to keep walking... Food first!")
            return self._leave("ForageFood")

        # Leveled up thanks to the elevations
        if context.level > self._entry_level:
            ai_logger.talk("[MapsToAlly] I leveled up! This rally is over for me.")
            return "SearchStone"

        for bcst in context.broadcasts:
            if bcst.content.level != self._entry_level:
                continue
            if bcst.content.msg_type == MessageType.ABORT:
                ai_logger.talk(
                    "[MapsToAlly] The rally was called off. Back to my own business."
                )
                return "SearchStone"
            if bcst.content.msg_type == MessageType.INCANT:
                if bcst.direction in BROADCAST_DIRECTION_ARRIVED:
                    ai_logger.talk(
                        "[MapsToAlly] The ritual is starting here! Holding still."
                    )
                    self.arrived = True
                    self.waiting_incant = True
                else:
                    ai_logger.talk(
                        "[MapsToAlly] The ritual started without me... too late."
                    )
                    return "SearchStone"

        if self.ticks_waited > RALLY_TIMEOUT:
            ai_logger.talk(
                "[MapsToAlly] I lost the signal... back to searching myself."
            )
            return self._leave("SearchStone")

        return None

    def get_action(self, context: DroneContext) -> str | None:
        """
        Emit a pending LEAVING, stay silent if the ritual is imminent,
        drop stones then say READY once on the rally tile, or follow
        the RALLY direction while travelling.
        """
        if self._leave_target and not self._leave_emitted:
            self._leave_emitted = True
            payload = BroadcastProtocol.encode(
                context.team_name,
                MessageType.LEAVING,
                self._entry_level,
                context.drone_id,
            )
            return f"Broadcast {payload}"

        if self.waiting_incant:
            return None

        # -- Already on the rally tile: drop stones and wait. --
        if self.arrived:
            if not context.vision:
                return "Look"
            stone = _next_stone_to_drop(context)
            if stone:
                return f"Set {stone}"
            if not self.ready_sent:
                self.ready_sent = True
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    MessageType.READY,
                    self._entry_level,
                    context.drone_id,
                )
                return f"Broadcast {payload}"
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
