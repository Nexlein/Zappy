##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## inventory_parser
##

from context import Inventory


def _parse_inventory(inventory_str: str) -> dict[str, int]:
    """
    Parses the raw Inventory response string from the server.

    Args:
        inventory_str (str): The raw response string: "[food 345, sibur 3, phiras 5]"

    Returns:
        dict[str, int]: A dictionary of resource names mapped to their count.
    """
    inventory_str = inventory_str.strip()
    if not (inventory_str.startswith("[") and inventory_str.endswith("]")):
        raise ValueError(
            "Invalid Inventory string format: must start with '[' and end with ']'"
        )

    content = inventory_str[1:-1].strip()
    if not content:
        return {}

    result = {}
    parts = content.split(",")
    for part in parts:
        part = part.strip()
        if not part:
            continue
        tokens = part.split()
        if len(tokens) != 2:
            raise ValueError(f"Invalid item entry in inventory: '{part}'")
        item_name = tokens[0].lower()
        try:
            item_count = int(tokens[1])
        except ValueError:
            raise ValueError(f"Invalid count for item '{item_name}': '{tokens[1]}'")
        result[item_name] = item_count

    return result


def update_inventory(inventory: Inventory, inventory_str: str) -> None:
    """
    Parses the inventory string and updates the given Inventory object in place.

    Args:
        inventory (Inventory): The drone's current inventory object to update.
        inventory_str (str): The raw response string.
    """
    parsed_data = _parse_inventory(inventory_str)

    # Update each attribute on the Inventory dataclass if it exists
    for item_name, count in parsed_data.items():
        if hasattr(inventory, item_name):
            setattr(inventory, item_name, count)
