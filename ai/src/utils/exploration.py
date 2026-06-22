##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## exploration helper
##

import random
from typing import Iterable
from context import DroneContext
from protocol.look_parser import find_closest_resource_path


def get_exploration_action(
    context: DroneContext,
    target_resources: Iterable[str],
    explore_turn_every: int = 5,
    forward_streak: int = 0,
) -> tuple[str | None, int]:
    """
    Evaluates the vision cone for specific resources. If found, builds a path queue.
    If not found, rotates randomly every N steps to avoid straight-line traps.

    Returns:
        (action_string, updated_forward_streak)
    """
    best_path = find_closest_resource_path(context.vision, target_resources)

    if best_path:
        context.path_queue.extend(best_path)
        return context.path_queue.pop(0), 0

    # No target resource visible — explore.
    new_streak = forward_streak + 1
    if new_streak % explore_turn_every == 0:
        return random.choice(["Right", "Left"]), new_streak

    return "Forward", new_streak
