from uai.behaviors.ABehavior import ABehavior
from context import DroneContext
from uai.state import UAIState

class ReproduceBehavior(ABehavior):
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        if state.reproduce_spawn_sent:
            state.last_fork_tick = context.total_ticks
            state.reset_reproduce_state()
            return None

        if state.reproduce_fork_sent:
            if context.last_command_successful:
                state.forks_done += 1
                state.last_fork_tick = context.total_ticks
            state.reset_reproduce_state()
            return None

        if not state.reproduce_connect_sent:
            state.reproduce_connect_sent = True
            return "Connect_nbr"

        if context.available_slots > 0:
            if not state.reproduce_spawn_sent:
                state.reproduce_spawn_sent = True
                return "Spawn_child"
            return None

        if not state.reproduce_fork_sent:
            state.reproduce_fork_sent = True
            return "Fork"

        return None
