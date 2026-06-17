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
    """True if `tile` holds enough players and EXACTLY the required stones."""
    if tile.player < PLAYERS_REQUIRED.get(level, 0):
        return False

    reqs = ELEVATION_REQUIREMENTS.get(level, {})
    all_stones = ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]

    for stone in all_stones:
        if getattr(tile, stone, 0) != reqs.get(stone, 0):
            return False
    return True


# ── Broadcast direction routing tables ---
# The server encodes the direction K of an incoming sound relative to the
# receiving player's current facing direction using the following grid:
#
#   8  1  2
#   7  0  3
#   6  5  4
#
# K=0: same tile (arrived).  K=1..8: compass directions relative to facing.
#
# These frozensets map K values to the movement action needed to approach the
# broadcaster. They are fixed protocol constants, not tunable parameters.

BROADCAST_DIRECTION_ARRIVED = frozenset({0})
BROADCAST_DIRECTION_FORWARD = frozenset({1, 2, 8})  # roughly ahead
BROADCAST_DIRECTION_RIGHT = frozenset({3, 4})  # right side / behind-right
BROADCAST_DIRECTION_LEFT = frozenset(
    {5, 6, 7}
)  # directly behind / left side / behind-left
