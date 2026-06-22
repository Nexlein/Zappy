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
        self.last_fork_tick = -9999
        self._connect_sent = False
        self._spawn_sent = False
        self._fork_sent = False

    def enter(self, context: DroneContext) -> None:
        self._connect_sent = False
        self._spawn_sent = False
        self._fork_sent = False

    def update(self, context: DroneContext) -> str | None:
        repr_cfg = get_reproduction_config()
        if context.forks_done >= repr_cfg.get("MAX_FORKS_PER_DRONE", 10):
            return "BroadcastHelp"
        if context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            return "ForageFood"

        if self._spawn_sent:
            self.last_fork_tick = context.total_ticks
            return "BroadcastHelp"

        if self._fork_sent:
            if context.last_command_successful:
                self.last_fork_tick = context.total_ticks
            return "BroadcastHelp"

        if self._connect_sent:
            return None

        # Wait 600 ticks (the time it takes for an egg to hatch) before allowing another fork.
        # This prevents the drone from spamming its entire max fork budget instantly.
        if context.total_ticks - self.last_fork_tick < 600:
            return "BroadcastHelp"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        repr_cfg = get_reproduction_config()
        if context.forks_done >= repr_cfg.get("MAX_FORKS_PER_DRONE", 10):
            return "Look"
        if context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            return "Look"
        if context.total_ticks - self.last_fork_tick < 600:
            return "Look"

        if not self._connect_sent:
            self._connect_sent = True
            return "Connect_nbr"

        if context.available_slots > 0:
            if not self._spawn_sent:
                self._spawn_sent = True
                return "Spawn_child"
            return None

        if not self._fork_sent:
            self._fork_sent = True
            return "Fork"
        return None

    def exit(self, context: DroneContext) -> None:
        pass
