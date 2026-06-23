BROADCAST_DIRECTION_ARRIVED = frozenset({0})
BROADCAST_DIRECTION_FORWARD = frozenset({1, 2, 8})  # roughly ahead
BROADCAST_DIRECTION_RIGHT = frozenset({6, 7})  # right side / behind-right
BROADCAST_DIRECTION_LEFT = frozenset(
    {3, 4, 5}
)  # directly behind / left side / behind-left


def get_action_for_broadcast(direction: int) -> str | None:
    """
    Returns the optimal movement command (Forward, Left, Right) to approach
    the source of a broadcast sound, based on the Zappy directional protocol.
    Returns None if already arrived (direction == 0).
    """
    if direction in BROADCAST_DIRECTION_ARRIVED:
        return None
    elif direction in BROADCAST_DIRECTION_FORWARD:
        return "Forward"
    elif direction in BROADCAST_DIRECTION_RIGHT:
        return "Right"
    elif direction in BROADCAST_DIRECTION_LEFT:
        return "Left"
    return "Forward"  # Fallback


def get_safe_navigation_action(
    action: str | None, last_turn_action: str | None
) -> tuple[str | None, str | None]:
    """
    Prevents the drone from getting stuck in an infinite Left/Right oscillation
    due to Zappy server bugs on Torus borders.
    Returns: (safe_action, new_last_turn_action)
    """
    if action in ("Left", "Right"):
        if last_turn_action and last_turn_action != action:
            return "Forward", None
        return action, action
    elif action == "Forward":
        return action, None
    return action, last_turn_action
