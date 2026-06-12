##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## AI Controller Factory
##

from fsm import AIController
from utility_ai import UtilityAIController
from context import DroneContext


def create_ai_controller(strategy: str, context: DroneContext):
    """Factory to instantiate the selected AI controller strategy."""
    if strategy == "fsm":
        return AIController(context)
    elif strategy == "utility":
        return UtilityAIController(context)
    else:
        raise ValueError(f"Unknown strategy strategy: {strategy}")
