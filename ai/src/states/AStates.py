##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## IStates
##

from abc import ABC, abstractmethod
from context import DroneContext


class State(ABC):
    """The abstract base class for all AI behaviors."""

    @abstractmethod
    def enter(self, context: DroneContext) -> str | None:
        """Called once when entering the state."""
        pass

    @abstractmethod
    def update(self, context: DroneContext) -> str | None:
        """Called every tick. Returns the name of the next state, or None."""
        pass

    @abstractmethod
    def exit(self, context: DroneContext) -> str | None:
        """Called once when leaving the state."""
        pass

    @abstractmethod
    def get_action(self, context: DroneContext) -> str | None:
        """Returns the Zappy server command (e.g., 'Forward', 'Take food')."""
        pass
