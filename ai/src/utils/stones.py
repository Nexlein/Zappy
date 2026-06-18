from context import Inventory, Tile
from utils.config_loader import get_evolution_config


def get_missing_stones(level: int, inventory: Inventory) -> dict[str, int]:
    """
    Calculates the exact stones required for the next elevation that the player
    does not currently have in their inventory.

    Returns a dictionary of {"stone_name": count_missing}
    """
    config = get_evolution_config()
    requirements = config.get("ELEVATION_REQUIREMENTS", {}).get(str(level))
    if not requirements:
        return {}

    missing = {}
    for stone, required_amount in requirements.items():
        current_amount = getattr(inventory, stone, 0)
        if current_amount < required_amount:
            missing[stone] = required_amount - current_amount
    return missing


def is_incantation_ready(level: int, tile: Tile) -> bool:
    """True if `tile` holds enough players and EXACTLY the required stones."""
    config = get_evolution_config()
    players_req = config.get("PLAYERS_REQUIRED", {}).get(str(level), 0)

    if tile.player < players_req:
        return False

    reqs = config.get("ELEVATION_REQUIREMENTS", {}).get(str(level), {})
    all_stones = ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]

    for stone in all_stones:
        if getattr(tile, stone, 0) != reqs.get(stone, 0):
            return False
    return True


def next_stone_to_drop(level: int, inventory: Inventory, tile: Tile) -> str | None:
    """
    Return the name of the first stone the drone should place on its current
    tile to meet the elevation requirements.
    """
    config = get_evolution_config()
    requirements = config.get("ELEVATION_REQUIREMENTS", {}).get(str(level), {})
    for stone, required in requirements.items():
        if getattr(tile, stone, 0) < required and getattr(inventory, stone, 0) > 0:
            return stone
    return None


def next_stone_to_take(level: int, tile: Tile) -> str | None:
    """Return the name of the first excess stone to remove from the tile."""
    config = get_evolution_config()
    reqs = config.get("ELEVATION_REQUIREMENTS", {}).get(str(level), {})
    all_stones = ["linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]
    for stone in all_stones:
        if getattr(tile, stone, 0) > reqs.get(stone, 0):
            return stone
    return None
