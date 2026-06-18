##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## context
##

from dataclasses import dataclass, field
from typing import List, Optional
import uuid
from BroadcastProtocol import DecodedBroadcast


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
    direction: int = -1


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
    drone_id: str = field(default_factory=lambda: str(uuid.uuid4()))

    # Dynamic drone state
    level: int = 1
    inventory: Inventory = field(default_factory=Inventory)

    # Global Tracker for the Swarm
    ally_roster: dict[str, AllyInfo] = field(default_factory=dict)

    # Vision snapshot from the last Look command.
    vision: List[Tile] = field(default_factory=list)

    # Incoming broadcast messages
    broadcasts: List[BroadcastMessage] = field(default_factory=list)

    # Queue of commands to reach a specific tile
    path_queue: List[str] = field(default_factory=list)

    # Reflects whether the LAST command the FSM issued succeeded.
    last_command_successful: Optional[bool] = None

    # Track ticks since last inventory command
    ticks_since_inventory: int = 0

    # True while a ritual freezes this drone
    elevation_in_progress: bool = False

    # Total ticks the drone has been active
    total_ticks: int = 0
