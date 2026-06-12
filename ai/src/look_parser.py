##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## look_parser
##

import math
from typing import Any, Dict, List
from context import Tile


def _parse_look(look_str: str) -> List[Dict[str, Any]]:
    """
    Parses the Look command response from the server.

    Args:
        look_str (str): The raw response string: "[player,,, thystame,, food,,,]"

    Returns:
        List[Dict[str, Any]]: A list of dictionaries representing the tiles in the vision cone.
                              Each dictionary contains relative coordinates (x, y) where:
                              - (0, 0) is the player's tile.
                              - y is the forward distance.
                              - x is the horizontal offset (negative is left, positive is right).
                              It also contains the count of each resource/players on the tile.
    """
    look_str = look_str.strip()
    if not (look_str.startswith("[") and look_str.endswith("]")):
        raise ValueError(
            "Invalid Look string format: must start with '[' and end with ']'"
        )

    content = look_str[1:-1]
    if not content.strip():
        return []

    tokens = content.split(",")
    result = []

    for i, token in enumerate(tokens):
        # Calculate relative coordinates
        # d is the row (distance forward, relative Y)
        # x_rel is the horizontal offset (relative X)
        d = int(math.isqrt(i))
        x_rel = i - d * d - d

        # Clean token and count resources
        tile_items = token.strip().split()

        tile_dict = {
            "index": i,
            "x": x_rel,
            "y": d,
            "player": 0,
            "food": 0,
            "linemate": 0,
            "deraumere": 0,
            "sibur": 0,
            "mendiane": 0,
            "phiras": 0,
            "thystame": 0,
        }

        for item in tile_items:
            item_lower = item.lower()
            if item_lower in tile_dict:
                tile_dict[item_lower] += 1

        result.append(tile_dict)

    return result


def parse_look_to_tiles(look_str: str) -> List[Tile]:
    """
    Parses the Look command response and returns a list of Tile dataclass instances.
    Suitable for populating DroneContext.vision.

    Args:
        look_str (str): The raw response string.

    Returns:
        List[Tile]: A list of Tile dataclass instances.
    """
    parsed_dicts = _parse_look(look_str)
    tiles = []
    for d in parsed_dicts:
        tiles.append(
            Tile(
                player=d["player"],
                food=d["food"],
                linemate=d["linemate"],
                deraumere=d["deraumere"],
                sibur=d["sibur"],
                mendiane=d["mendiane"],
                phiras=d["phiras"],
                thystame=d["thystame"],
            )
        )
    return tiles


def generate_path_to_tile(index: int) -> List[str]:
    """
    Returns a sequence of commands to reach a specific tile index.
    To reach a tile, we must move forward first, then turn, then move forward.

    Vision indexes:
    0: self
    1: left-front, 2: front, 3: right-front
    4: left-left-front, 5: left-front, 6: front, 7: right-front, 8: right-right-front
    """
    if index == 0:
        return []

    row = math.isqrt(index)
    x_offset = index - row * row - row

    path = ["Forward"] * row
    if x_offset < 0:
        path.append("Left")
        path.extend(["Forward"] * abs(x_offset))
    elif x_offset > 0:
        path.append("Right")
        path.extend(["Forward"] * x_offset)

    return path
