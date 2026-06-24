##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Reproduction Layer (population growth)
##

from queen.states.AState import AState
from queen.states.StateNames import AIState
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
        self._connect_sent = False
        self._spawn_sent = False
        self._fork_sent = False

    def enter(self, context: DroneContext) -> None:
        self._connect_sent = False
        self._spawn_sent = False
        self._fork_sent = False

    def update(self, context: DroneContext) -> str | None:
        if not context.is_queen:
            return AIState.SEARCH_STONE

        if context.target_forks == -1:
            context.target_forks = max(0, 5 - len(context.ally_roster))

        if context.forks_done >= context.target_forks:
            return AIState.SEARCH_STONE

        repr_cfg = get_reproduction_config()
        if context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            return AIState.FORAGE_FOOD

        if self._spawn_sent:
            return AIState.FORAGE_FOOD

        if self._fork_sent:
            return AIState.FORAGE_FOOD

        if self._connect_sent:
            return None

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if not context.is_queen:
            return "Look"

        if context.target_forks == -1:
            context.target_forks = max(0, 5 - len(context.ally_roster))

        if context.forks_done >= context.target_forks:
            return "Look"

        repr_cfg = get_reproduction_config()
        if context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
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
