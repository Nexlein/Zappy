##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## fsm
##

from context import DroneContext
from config import INVENTORY_REFRESH_INTERVAL
from fsm.states.AStates import State
from fsm.states.survival import ForageFood
from fsm.states.evolution import SearchStone, IncantationState
from fsm.states.swarm import BroadcastHelp, MapsToAlly
from fsm.states.reproduce import Reproduce
from ai_logger import ai_logger


class AIController:
    """
    The Finite State Machine controller for the Zappy AI drone.

    Tick contract (called once per main-loop iteration):
      1. update()     — evaluates transition conditions and switches state if needed.
      2. get_action() — returns the next server command string, or None to idle.
    """

    def __init__(self, initial_context: DroneContext):
        self.context = initial_context

        self.states: dict[str, State] = {
            "ForageFood": ForageFood(),
            "SearchStone": SearchStone(),
            "BroadcastHelp": BroadcastHelp(),
            "MapsToAlly": MapsToAlly(),
            "Incantation": IncantationState(),
            "Reproduce": Reproduce(),
        }

        self.current_state_name = "ForageFood"
        self.current_state = self.states[self.current_state_name]
        self.current_state.enter(self.context)

    def tick(self) -> str | None:
        """Evaluate state logic and return the next command to send, or None."""
        if not self.current_state:
            return None

        # 1. Check for a state transition. Must run every tick: it is also
        # where states consume context.broadcasts (cleared after each tick).
        next_state_name = self.current_state.update(self.context)
        if next_state_name and next_state_name != self.current_state_name:
            self._transition_to(next_state_name)

        # 2. Periodic inventory refresh: preempts the action, not the update.
        if self.context.ticks_since_inventory >= INVENTORY_REFRESH_INTERVAL:
            self.context.ticks_since_inventory = 0
            return "Inventory"
        self.context.ticks_since_inventory += 1

        # 3. Ask the (possibly new) state for the next action
        action = self.current_state.get_action(self.context)
        ai_logger.log_state(self.current_state_name, action or "None", self.context)
        return action

    def _transition_to(self, new_state_name: str) -> None:
        """Tear down the current state and set up the new one."""
        ai_logger.info(f"[FSM] {self.current_state_name} -> {new_state_name}")

        self.current_state.exit(self.context)
        self.current_state_name = new_state_name
        self.current_state = self.states[new_state_name]
        self.context.path_queue.clear()
        self.current_state.enter(self.context)
