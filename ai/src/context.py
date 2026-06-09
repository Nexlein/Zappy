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
    The shared state object modified by the network loop and read by the FSM.
    """

    # Static server info (populated at handshake)
    team_name: str = ""
    map_width: int = 0
    map_height: int = 0
    available_slots: int = 0

    # Dynamic drone state
    level: int = 1
    inventory: Inventory = field(default_factory=Inventory)

    # Vision snapshot from the last Look command.
    vision: List[Tile] = field(default_factory=list)

    # Incoming broadcast messages
    broadcasts: List[BroadcastMessage] = field(default_factory=list)

    # Reflects whether the LAST command the FSM issued succeeded.
    last_command_successful: Optional[bool] = None
