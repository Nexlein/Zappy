from utils.stones import next_stone_to_take, next_stone_to_drop
from utils.config_loader import get_evolution_config, get_swarm_config
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType
from uai.behaviors.ABehavior import ABehavior
from context import DroneContext
from uai.state import UAIState


class RallyBehavior(ABehavior):
    def get_action(self, context: DroneContext, state: UAIState) -> str | None:
        if not context.vision:
            return "Look"

        evo_cfg = get_evolution_config()
        players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(str(context.level), 0)
        if state.ready_count + 1 >= players_req:
            stone_to_take = next_stone_to_take(context.level, context.vision[0])
            if stone_to_take:
                return f"Take {stone_to_take}"

            stone = next_stone_to_drop(
                context.level, context.inventory, context.vision[0]
            )
            if stone:
                return f"Set {stone}"

        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            swarm_cfg = get_swarm_config()
            if state.tick_since_bcast >= swarm_cfg.get("BCAST_INTERVAL", 2):
                state.tick_since_bcast = 0
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    MessageType.RALLY,
                    context.level,
                    context.drone_id,
                )
                return f"Broadcast {payload}"

        state.tick_since_bcast += 1

        if state.ready_count + 1 >= players_req or state.tick_since_bcast % 3 == 0:
            return "Look"
        return None
