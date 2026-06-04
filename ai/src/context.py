##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## context
##

from dataclasses import dataclass, field
from typing import List, Optional


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
    text: str  # The raw decoded string payload


@dataclass
class DroneContext:
    """
    The shared state modified by the network and read by the FSM.
    Dev A (Network) updates this object.
    Dev B (FSM) reads this object to make decisions.
    """

    # Static server info (populated at handshake)
    team_name: str = ""
    map_width: int = 0
    map_height: int = 0
    available_slots: int = 0

    # Dynamic drone state
    level: int = 1
    inventory: Inventory = field(default_factory=Inventory)
    ticks_since_inventory: int = 999

    # Vision is a 1D list where index corresponds to the tile number (0 is current tile)
    vision: List[Tile] = field(default_factory=list)

    # Event queues populated by the network loop
    broadcasts: List[BroadcastMessage] = field(default_factory=list)

    # Track the success/failure of the last command sent ('ok' / 'ko')
    last_command_successful: Optional[bool] = None
