##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## AI Controller Factory
##

from fsm.controller import AIController
from uai.controller import UtilityAIController
from context import DroneContext


def create_ai_controller(strategy: str, context: DroneContext):
    """Factory to instantiate the selected AI controller strategy."""
    if strategy == "fsm":
        return AIController(context)
    elif strategy in ("utility", "uai"):
        return UtilityAIController(context)
    else:
        raise ValueError(f"Unknown strategy: {strategy}")
