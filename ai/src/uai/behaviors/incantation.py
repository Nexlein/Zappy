from utils.config_loader import get_evolution_config
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType
from uai.behaviors.ABehavior import ABehavior
from context import DroneContext
from uai.state import UAIState


class IncantationBehavior(ABehavior):
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        if state.incant_cmd_sent and context.last_command_successful is False:
            state.leader_aborted = True
            # In old controller, this recursive tick() would be handled by returning an abort in controller loop
            return None

        evo_cfg = get_evolution_config()
        if not state.incant_bcast_sent and context.level > evo_cfg.get(
            "SOLO_INCANTATION_LEVEL", 1
        ):
            state.incant_bcast_sent = True
            payload = BroadcastProtocol.encode(
                context.team_name,
                MessageType.INCANT,
                context.level,
                context.drone_id,
            )
            return f"Broadcast {payload}"

        if not state.incant_cmd_sent:
            state.incant_cmd_sent = True
            return "Incantation"
        return None
