import random
from elevations import (
    ELEVATION_REQUIREMENTS,
    BROADCAST_DIRECTION_ARRIVED,
    BROADCAST_DIRECTION_FORWARD,
    BROADCAST_DIRECTION_RIGHT,
    BROADCAST_DIRECTION_LEFT,
)
from config import BCAST_INTERVAL, SOLO_INCANTATION_LEVEL
from BroadcastProtocol import BroadcastProtocol, MessageType
from look_parser import generate_path_to_tile

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from context import DroneContext


class ActionGenerators:
    """Generates commands based on the selected AI behavior."""

    if TYPE_CHECKING:
        context: "DroneContext"
        _forward_streak: int
        incant_cmd_sent: bool
        leader_aborted: bool
        incant_bcast_sent: bool
        tick_since_bcast: int
        arrived: bool
        ready_sent: bool
        highest_rally_direction: int | None

        def tick(self) -> str | None: ...

    def _get_missing_stones(self) -> dict[str, int]:
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        return {
            stone: required - getattr(self.context.inventory, stone, 0)
            for stone, required in requirements.items()
            if getattr(self.context.inventory, stone, 0) < required
        }

    def _next_stone_to_drop(self) -> str | None:
        if not self.context.vision:
            return None
        tile = self.context.vision[0]
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        for stone, required in requirements.items():
            if (
                getattr(tile, stone, 0) < required
                and getattr(self.context.inventory, stone, 0) > 0
            ):
                return stone
        return None

    def _get_survival_action(self) -> str:
        if self.context.path_queue:
            return self.context.path_queue.pop(0)
        if not self.context.vision:
            return "Look"
        if self.context.vision[0].food > 0:
            self._forward_streak = 0
            return "Take food"
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            if tile.food > 0:
                self._forward_streak = 0
                path = generate_path_to_tile(i)
                if path:
                    self.context.path_queue.extend(path)
                    return self.context.path_queue.pop(0)
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_gather_action(self) -> str:
        if self.context.path_queue:
            return self.context.path_queue.pop(0)
        if not self.context.vision:
            return "Look"
        missing = self._get_missing_stones()
        current_tile = self.context.vision[0]
        for stone in missing:
            if getattr(current_tile, stone, 0) > 0:
                self._forward_streak = 0
                return f"Take {stone}"
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            for stone in missing:
                if getattr(tile, stone, 0) > 0:
                    self._forward_streak = 0
                    path = generate_path_to_tile(i)
                    if path:
                        self.context.path_queue.extend(path)
                        return self.context.path_queue.pop(0)
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_incantation_action(self) -> str | None:
        # Check if previous incantation failed
        if self.incant_cmd_sent and self.context.last_command_successful is False:
            self.leader_aborted = True
            return self.tick()  # trigger abort

        if not self.incant_bcast_sent and self.context.level > SOLO_INCANTATION_LEVEL:
            self.incant_bcast_sent = True
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.INCANT,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if not self.incant_cmd_sent:
            self.incant_cmd_sent = True
            return "Incantation"
        return None

    def _get_rally_action(self) -> str:
        if not self.context.vision:
            return "Look"
        stone = self._next_stone_to_drop()
        if stone:
            return f"Set {stone}"
        if self.tick_since_bcast >= BCAST_INTERVAL:
            self.tick_since_bcast = 0
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.RALLY,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"
        self.tick_since_bcast += 1
        return "Look"

    def _get_follow_action(self) -> str:
        direction = self.highest_rally_direction
        
        if direction is not None:
            if direction != 0 and self.arrived:
                # Leader moved or changed
                self.arrived = False
                self.ready_sent = False
                
            if not self.arrived:
                if direction in BROADCAST_DIRECTION_ARRIVED:
                    self.arrived = True
                elif direction in BROADCAST_DIRECTION_FORWARD:
                    return "Forward"
                elif direction in BROADCAST_DIRECTION_RIGHT:
                    return "Right"
                elif direction in BROADCAST_DIRECTION_LEFT:
                    return "Left"

        if self.arrived:
            if not self.context.vision:
                return "Look"
            stone = self._next_stone_to_drop()
            if stone:
                return f"Set {stone}"
            if not self.ready_sent:
                self.ready_sent = True
                payload = BroadcastProtocol.encode(
                    self.context.team_name,
                    MessageType.READY,
                    self.context.level,
                    self.context.drone_id,
                )
                return f"Broadcast {payload}"
            return "Look"

        return "Look"
