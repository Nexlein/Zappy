##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## fsm
##

from context import DroneContext
from states.AStates import State
from states.survival import ForageFood
from states.evolution import SearchStone, IncantationState
from states.swarm import BroadcastHelp, MapsToAlly


class AIController:
    def __init__(self, initial_context: DroneContext):
        self.context = initial_context

        # We will register our states here as we build them
        self.states: dict[str, State] = {
            "ForageFood": ForageFood(),
            "SearchStone": SearchStone(),
            "BroadcastHelp": BroadcastHelp(),
            "MapsToAlly": MapsToAlly(),
            "Incantation": IncantationState(),
        }

        self.current_state_name = "ForageFood"
        self.current_state = self.states[self.current_state_name]
        self.current_state.enter(self.context)

    def tick(self) -> str | None:
        """Evaluates logic and returns the next command to send."""
        if not self.current_state:
            return None

        # 1. Check for transitions
        next_state_name = self.current_state.update(self.context)

        if next_state_name and next_state_name != self.current_state_name:
            self._transition_to(next_state_name)

        # 2. Check if we need to refresh inventory
        if self.context.ticks_since_inventory >= 15:
            self.context.ticks_since_inventory = 0
            return "Inventory"

        # 3. Get the action for the current state
        action = self.current_state.get_action(self.context)
        if action != "Inventory":
            self.context.ticks_since_inventory += 1
        else:
            self.context.ticks_since_inventory = 0
        return action

    def _transition_to(self, new_state_name: str):
        """Safely tears down the old state and sets up the new one."""
        print(f"[FSM] Transitioning: {self.current_state_name} -> {new_state_name}")

        if self.current_state:
            self.current_state.exit(self.context)

        self.current_state_name = new_state_name
        self.current_state = self.states[self.current_state_name]
        self.current_state.enter(self.context)
