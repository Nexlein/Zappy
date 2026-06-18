##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Reproduction Layer (population growth)
##

from fsm.states.AState import AState
from context import DroneContext
from utils.config_loader import get_reproduction_config


class Reproduce(AState):
    """
    Reproduction state — grow the team for the high-level incantations.

    Slot-gated policy: only lay an egg if no idle egg already waits for a
    client, so growth self-serialises (after a Fork, available_slots stays at 1
    until the spawned client occupies the egg, blocking further forks).

    Sequence (one command per tick — pending_command gates the controller, so
    each reply is processed before the next update/get_action):
      1. Connect_nbr  — refresh available_slots (handshake value is stale).
      2. decide       — slots > 0 ? an egg is idle, bail out. slots == 0 ? fork.
      3. Fork         — orchestrator spawns the client + reaps it later.

    forks_done lives on the instance (states are singletons in the controller),
    giving each drone a lifetime fork budget.
    """

    def __init__(self) -> None:
        self.forks_done = 0
        self.last_fork_tick = -9999

    def enter(self, context: DroneContext) -> None:
        self._fork_sent = False

    def update(self, context: DroneContext) -> str | None:
        repr_cfg = get_reproduction_config()
        if self.forks_done >= repr_cfg.get("MAX_FORKS_PER_DRONE", 10):
            return "SearchStone"
        if context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            return "SearchStone"

        if self._fork_sent:
            if context.last_command_successful:
                self.forks_done += 1
                self.last_fork_tick = context.total_ticks
            return "SearchStone"

        if context.total_ticks - self.last_fork_tick < 800:
            return "SearchStone"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not self._fork_sent:
            self._fork_sent = True
            return "Fork"
        return None

    def exit(self, context: DroneContext) -> None:
        pass
