##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## fsm
##

from context import DroneContext
from utils.config_loader import get_survival_config
from fsm.states.AState import AState
from fsm.states.survival import ForageFood
from fsm.states.evolution import SearchStone, IncantationState
from fsm.states.swarm import BroadcastHelp, MapsToAlly
from fsm.states.reproduce import Reproduce
from fsm.states.StateNames import AIState
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

        self.states: dict[str, AState] = {
            AIState.FORAGE_FOOD: ForageFood(),
            AIState.SEARCH_STONE: SearchStone(),
            AIState.BROADCAST_HELP: BroadcastHelp(),
            AIState.MAPS_TO_ALLY: MapsToAlly(),
            AIState.INCANTATION: IncantationState(),
            AIState.REPRODUCE: Reproduce(),
        }

        self.current_state_name = AIState.FORAGE_FOOD
        self.current_state = self.states[self.current_state_name]
        self.current_state.enter(self.context)

        self.context.is_queen = False
        self.boot_tick = 0

    def tick(self) -> str | None:
        """Evaluate state logic and return the next command to send, or None."""
        if not self.current_state:
            return None

        # 1. Update Context (Broadcasts & Inventory Handled by Orchestrator)
        self.context.total_ticks += 1
        
        # Queen Election
        if self.context.total_ticks >= 20:
            is_oldest = True
            for drone_id in self.context.ally_roster.keys():
                if drone_id < self.context.drone_id:
                    is_oldest = False
                    break
            self.context.is_queen = is_oldest

        # 1. Check for a state transition. Must run every tick: it is also
        # where states consume context.broadcasts (cleared after each tick).
        next_state_name = self.current_state.update(self.context)
        if next_state_name and next_state_name != self.current_state_name:
            self._transition_to(next_state_name)

        # 2. Periodic inventory refresh: preempts the action, not the update.
        inventory_refresh_interval = get_survival_config().get(
            "INVENTORY_REFRESH_INTERVAL", 15
        )
        if self.context.total_ticks % inventory_refresh_interval == 0:
            return "Inventory"

        # 3. Update global inventory
        if self.context.total_ticks % 10 == 0:
            from protocol.BroadcastProtocol import BroadcastProtocol, MessageType

            tail = f"linemate:{self.context.inventory.linemate},deraumere:{self.context.inventory.deraumere},sibur:{self.context.inventory.sibur},mendiane:{self.context.inventory.mendiane},phiras:{self.context.inventory.phiras},thystame:{self.context.inventory.thystame},food:{self.context.inventory.food}"
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.SWARM_INVENTORY,
                self.context.level,
                self.context.drone_id,
                tail,
            )
            return f"Broadcast {payload}"

        # 3. Ask the (possibly new) state for the next action
        action = self.current_state.get_action(self.context)
        ai_logger.log_state(
            self.current_state_name,
            action or "None",
            self.context.level,
            self.context.inventory,
        )
        return action

    def _transition_to(self, new_state_name: str) -> None:
        """Tear down the current state and set up the new one."""

        self.current_state.exit(self.context)
        self.current_state_name = new_state_name
        self.current_state = self.states[new_state_name]
        self.context.path_queue.clear()
        self.current_state.enter(self.context)
