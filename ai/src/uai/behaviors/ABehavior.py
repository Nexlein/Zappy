from abc import ABC, abstractmethod
from context import DroneContext
from uai.state import UAIState


class ABehavior(ABC):
    @abstractmethod
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        pass
