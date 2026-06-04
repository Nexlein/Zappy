##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Swarm Layer (Contextual Priority)
##

from states.AStates import State
from context import DroneContext
from elevations import is_incantation_ready, ELEVATION_REQUIREMENTS
from config import SURVIVAL_THRESHOLD, RALLY_TIMEOUT, BCAST_INTERVAL
from BroadcastProtocol import BroadcastProtocol, MessageType, DecodedBroadcast

# Contextual Priority

# From Level 2 onwards, evolution requires teammates.
# The AI must pause its solo evolution to interact with the network.


def _next_stone_to_drop(context: DroneContext) -> str | None:
    tile = context.vision[0]
    for stone, required in ELEVATION_REQUIREMENTS.get(context.level, {}).items():
        if getattr(tile, stone, 0) < required and getattr(context.inventory, stone, 0) > 0:
            return stone
    return None


# BroadcastHelp: Yelling across the map for teammates.
class BroadcastHelp(State):
    """The BroadcastHelp state: Priority 4 - Handles calling teammates for assistance when evolving."""
    def enter(self, context: DroneContext) -> str | None:
        self.ticks_waited = 0
        self.dropped = False
        self.tick_since_bcast = 999
        context.vision = []
        print("[BroadcastHelp] Entering state. Broadcasting for help.")
        return "BroadcastHelp"

    def update(self, context: DroneContext) -> str | None:
        self.ticks_waited += 1

        if context.vision:
            tile = context.vision[0]
            if is_incantation_ready(context.level, tile):
                return "Incantation"
        elif context.inventory.food < SURVIVAL_THRESHOLD:
            return "ForageFood"
        elif self.ticks_waited > RALLY_TIMEOUT:
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not context.vision:
            return "Look"

        stone = _next_stone_to_drop(context)
        if stone:
            context.vision = []
            return f"Set {stone}"

        if self.tick_since_bcast >= BCAST_INTERVAL:
            self.tick_since_bcast = 0
            payload = BroadcastProtocol.encode(context.team_name, MessageType.RALLY, context.level)
            return f"Broadcast {payload}"

        self.tick_since_bcast += 1
        context.vision = []
        return "Look"

    def exit(self, context: DroneContext) -> str | None:
        print("[BroadcastHelp] Leaving rally. Allies grouped or aborting.")
        return None


# MapsToAlly: Following a broadcast direction (K) to group up on the same tile.
class MapsToAlly(State):
    """The MapsToAlly state: Priority 3 - Handles following broadcast directions to group up."""
    def enter(self, context: DroneContext) -> str | None:
        print("[MapsToAlly] Entering state. Following broadcast direction.")
        self.level = context.level
        self.ticks_waited = 0
        self.arrived = False
        return "MapsToAlly"

    def update(self, context: DroneContext) -> str | None:
        self.ticks_waited += 1

        if context.inventory.food < SURVIVAL_THRESHOLD:
            return "ForageFood"
        elif context.level > self.level or self.ticks_waited > RALLY_TIMEOUT:
            return "SearchStone"
        return None

    def get_action(self, context: DroneContext) -> str | None:
        if self.arrived:
            if not context.vision:
                return "Look"
            stone = _next_stone_to_drop(context)
            if stone:
                context.vision = []
                return f"Set {stone}"
            context.vision = []
            return "Look"

        # Still travelling: follow the latest matching RALLY direction.
        for bcst in context.broadcasts:
            decoded = BroadcastProtocol.decode(bcst.text)
            if decoded and decoded.msg_type == MessageType.RALLY and decoded.level == self.level:
                direction = bcst.direction
                if direction == 0:
                    self.arrived = True
                    return "Look"
                elif direction in (1, 2, 8):
                    return "Forward"
                elif direction in (5, 6, 7):
                    return "Right"
                elif direction in (3, 4):
                    return "Left"

        # No RALLY heard this tick: stay put rather than returning None.
        return "Look"

    def exit(self, context: DroneContext) -> str | None:
        print("[MapsToAlly] Leaving group-up.")
        return None

