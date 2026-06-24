from utils.navigation import get_action_for_broadcast
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType
from uai.behaviors.ABehavior import ABehavior
from context import DroneContext
from uai.state import UAIState


class FollowBehavior(ABehavior):
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        direction = state.highest_rally_direction

        if direction is not None:
            if direction != 0 and state.arrived:
                state.arrived = False
                state.ready_sent = False

            if not state.arrived:
                action = get_action_for_broadcast(direction)
                if action is None:
                    state.arrived = True
                else:
                    return action

        if state.arrived:
            if not state.ready_sent:
                state.ready_sent = True
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    MessageType.READY,
                    context.level,
                    context.drone_id,
                )
                return f"Broadcast {payload}"
            return None

        return None
