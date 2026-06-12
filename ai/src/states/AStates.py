##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Abstract base class for all AI states
##

from abc import ABC, abstractmethod
from context import DroneContext


class State(ABC):
    """Abstract base class for all Zappy AI state behaviours."""

    @abstractmethod
    def enter(self, context: DroneContext) -> None:
        """
        Called exactly once when this state becomes active.
        Use for initialisation (reset counters, print diagnostics, etc.).
        Does NOT return a command; the first command comes from get_action().
        """
        pass

    @abstractmethod
    def update(self, context: DroneContext) -> str | None:
        """
        Called once per FSM tick BEFORE get_action().
        Evaluates transition conditions and returns the name of the next state,
        or None to stay in the current state.
        """
        pass

    @abstractmethod
    def get_action(self, context: DroneContext) -> str | None:
        """
        Called once per FSM tick AFTER update().
        Returns the Zappy server command string to send (e.g. 'Forward',
        'Take food', 'Incantation'), or None to idle this tick.
        """
        pass

    @abstractmethod
    def exit(self, context: DroneContext) -> None:
        """
        Called exactly once when this state is about to be replaced.
        Use for clean-up and diagnostics.
        """
        pass
