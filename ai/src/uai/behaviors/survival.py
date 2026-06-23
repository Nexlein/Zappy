import random
from protocol.look_parser import find_closest_resource_path
from uai.behaviors.ABehavior import ABehavior
from context import DroneContext
from uai.state import UAIState


class SurvivalBehavior(ABehavior):
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        if context.path_queue:
            return context.path_queue.pop(0)
        if not context.vision:
            return "Look"
        if context.vision[0].food > 0:
            state.forward_streak = 0
            return "Take food"

        best_path = find_closest_resource_path(context.vision, ["food"])

        if best_path:
            state.forward_streak = 0
            context.path_queue.extend(best_path)
            return context.path_queue.pop(0)
        state.forward_streak += 1
        if state.forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"
