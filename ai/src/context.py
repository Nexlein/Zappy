##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## context
##

from dataclasses import dataclass, field
from typing import List, Optional
import uuid
import time
from protocol.BroadcastProtocol import DecodedBroadcast, MessageType


@dataclass
class Inventory:
    """Represents the drone's current pockets and life supply."""

    food: int = 0
    linemate: int = 0
    deraumere: int = 0
    sibur: int = 0
    mendiane: int = 0
    phiras: int = 0
    thystame: int = 0


@dataclass
class Tile:
    """Represents a single tile parsed from the Look command."""

    player: int = 0
    food: int = 0
    linemate: int = 0
    deraumere: int = 0
    sibur: int = 0
    mendiane: int = 0
    phiras: int = 0
    thystame: int = 0


@dataclass
class BroadcastMessage:
    """Represents an incoming sound/message from another player."""

    direction: int  # 0 (same tile) to 8.
    content: DecodedBroadcast


@dataclass
class AllyInfo:
    """Stores known information about a teammate."""

    level: int
    last_seen_tick: int
    is_ready: bool = False
    is_rallying: bool = False
    is_coming: bool = False
    direction: int = -1
    inventory: dict[str, int] = field(default_factory=dict)


@dataclass
class DroneContext:
    """
    The shared state object modified by the network loop and read by the FSM.
    """

    # Static server info (populated at handshake)
    team_name: str = ""
    map_width: int = 0
    map_height: int = 0
    available_slots: int = 0
    drone_id: str = field(
        default_factory=lambda: f"{int(time.time() * 1000)}_{uuid.uuid4().hex[:8]}"
    )

    # Dynamic drone state
    level: int = 1
    inventory: Inventory = field(default_factory=Inventory)

    # Global Tracker for the Swarm
    ally_roster: dict[str, AllyInfo] = field(default_factory=dict)

    # Role tracking
    is_queen: bool = False

    # Vision snapshot from the last Look command.
    vision: List[Tile] = field(default_factory=list)

    # Incoming broadcast messages
    broadcasts: List[BroadcastMessage] = field(default_factory=list)

    # Queue of commands to reach a specific tile
    path_queue: List[str] = field(default_factory=list)

    # Reflects whether the LAST command the FSM issued succeeded.
    last_command_successful: Optional[bool] = None

    # True while a ritual freezes this drone
    elevation_in_progress: bool = False

    # Total ticks the drone has been active
    total_ticks: int = 0

    # Ticks since last inventory refresh
    ticks_since_inventory: int = 0

    # How many forks this drone has successfully completed
    forks_done: int = 0

    # Calculated number of forks to reach swarm size 6
    target_forks: int = -1

    def update_roster(self) -> None:
        """Process broadcasts to update teammate state and cull inactive members."""
        for bcst in self.broadcasts:
            if bcst.content.drone_id and bcst.content.drone_id != self.drone_id:
                info = self.ally_roster.get(bcst.content.drone_id)
                if info is None:
                    info = AllyInfo(
                        level=bcst.content.level,
                        last_seen_tick=self.total_ticks,
                    )
                    self.ally_roster[bcst.content.drone_id] = info
                else:
                    info.level = bcst.content.level
                    info.last_seen_tick = self.total_ticks

                info.direction = bcst.direction

                if bcst.content.msg_type in (MessageType.RALLY, MessageType.RALLY_FULL):
                    info.is_rallying = True
                    info.is_ready = False
                    info.is_coming = False
                elif bcst.content.msg_type == MessageType.READY:
                    info.is_ready = True
                    info.is_coming = False
                elif bcst.content.msg_type == MessageType.COMING:
                    info.is_ready = False
                    info.is_coming = True
                elif bcst.content.msg_type in (
                    MessageType.LEAVING,
                    MessageType.ABORT,
                    MessageType.INCANT,
                ):
                    info.is_ready = False
                    info.is_rallying = False
                    info.is_coming = False
                elif bcst.content.msg_type == MessageType.SWARM_INVENTORY:
                    if bcst.content.tail:
                        try:
                            for item in bcst.content.tail.split(","):
                                k, v = item.split(":")
                                info.inventory[k] = int(v)
                        except ValueError:
                            pass

        dead_allies = [
            drone_id
            for drone_id, info in self.ally_roster.items()
            if self.total_ticks - info.last_seen_tick > 2000
        ]
        for dead_id in dead_allies:
            del self.ally_roster[dead_id]
