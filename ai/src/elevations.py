##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Elevation requirements (shared by the evolution and swarm states)
##

# Players of the same level required on the tile to go from `level` to `level + 1`.
PLAYERS_REQUIRED = {1: 1, 2: 2, 3: 2, 4: 4, 5: 4, 6: 6, 7: 6}

# Stones required on the tile to go from `level` to `level + 1`.
ELEVATION_REQUIREMENTS = {
    1: {
        "linemate": 1,
        "deraumere": 0,
        "sibur": 0,
        "mendiane": 0,
        "phiras": 0,
        "thystame": 0,
    },
    2: {
        "linemate": 1,
        "deraumere": 1,
        "sibur": 1,
        "mendiane": 0,
        "phiras": 0,
        "thystame": 0,
    },
    3: {
        "linemate": 2,
        "deraumere": 0,
        "sibur": 1,
        "mendiane": 0,
        "phiras": 2,
        "thystame": 0,
    },
    4: {
        "linemate": 1,
        "deraumere": 1,
        "sibur": 2,
        "mendiane": 0,
        "phiras": 1,
        "thystame": 0,
    },
    5: {
        "linemate": 1,
        "deraumere": 2,
        "sibur": 1,
        "mendiane": 3,
        "phiras": 0,
        "thystame": 0,
    },
    6: {
        "linemate": 1,
        "deraumere": 2,
        "sibur": 3,
        "mendiane": 0,
        "phiras": 1,
        "thystame": 0,
    },
    7: {
        "linemate": 2,
        "deraumere": 2,
        "sibur": 2,
        "mendiane": 2,
        "phiras": 2,
        "thystame": 1,
    },
}


def is_incantation_ready(level, tile) -> bool:
    """True if `tile` holds enough players and stones to start the elevation."""
    if tile.players < PLAYERS_REQUIRED.get(level, 0):
        return False
    for stone, amount in ELEVATION_REQUIREMENTS.get(level, {}).items():
        if getattr(tile, stone, 0) < amount:
            return False
    return True
