##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Evolution Layer (Medium Priority)
##

from utils.stones import get_megaritual_missing_stones, get_global_inventory
from utils.config_loader import get_survival_config, get_evolution_config
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType
from context import DroneContext
from utils.exploration import get_exploration_action

from fsm.states.AState import AState
from fsm.states.StateNames import AIState


class SearchStone(AState):
    """
    Evolution state — Priority 2: Gather the stones required to level up.

    Vision contract:
      - Does NOT clear context.vision manually.
      - After 'Take <stone>' the main loop decrements vision[0].<stone> in place,
        so further takes on the same tile work without an extra Look.
    """

    def enter(self, context: DroneContext) -> None:
        self._forward_streak = 0

    def update(self, context: DroneContext) -> str | None:
        evo_cfg = get_evolution_config()
        surv_cfg = get_survival_config()

        if context.level >= evo_cfg.get("MAX_LEVEL", 8):
            return AIState.FORAGE_FOOD

        if context.inventory.food < surv_cfg.get("SAFE_FOOD_THRESHOLD", 15):
            return AIState.FORAGE_FOOD

        # Rush behavior: The Queen must spawn exactly 5 children before doing ANY rituals
        if context.is_queen and len(context.ally_roster) < 5:
            return AIState.REPRODUCE

        # Parse SWARM_INVENTORY broadcasts to build a global inventory
        global_inventory = get_global_inventory(context)

        # React to a teammate's RALLY call (solo levels can incant alone).
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            if any(
                info.is_rallying and info.level == context.level
                for info in context.ally_roster.values()
            ):
                return AIState.MAPS_TO_ALLY

        missing = get_megaritual_missing_stones(context.level, global_inventory)

        if not missing:
            # Check if entire swarm is well-fed before rallying
            # They need ~3 food per remaining ritual to survive
            required_food_per_drone = (
                evo_cfg.get("MAX_LEVEL", 8) - context.level
            ) * 3 + 5

            if global_inventory.get("food", 0) < required_food_per_drone * (
                len(context.ally_roster) + 1
            ):
                return AIState.FORAGE_FOOD
            return AIState.BROADCAST_HELP

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if context.path_queue:
            return context.path_queue.pop(0)

        if not context.vision:
            return "Look"

        global_inventory = get_global_inventory(context)

        missing_stones = get_megaritual_missing_stones(context.level, global_inventory)

        current_tile = context.vision[0]

        # Pick up any needed stone on the current tile, unless there are other players here (could be a ritual).
        if current_tile.player <= 1:
            for stone in missing_stones:
                if getattr(current_tile, stone, 0) > 0:
                    self._forward_streak = 0
                    return f"Take {stone}"

        surv_cfg = get_survival_config()
        action, self._forward_streak = get_exploration_action(
            context,
            missing_stones,
            surv_cfg.get("EXPLORE_TURN_EVERY", 5),
            self._forward_streak,
        )
        return action

    def exit(self, context: DroneContext) -> None:
        pass


class IncantationState(AState):
    """
    Evolution state: Execute the incantation ritual.
    """

    def enter(self, context: DroneContext) -> None:
        self.broadcast_sent = False
        self.command_sent = False
        self.need_abort = False
        self.abort_sent = False

    def update(self, context: DroneContext) -> str | None:
        """
        Read the ritual verdict; on a failed group ritual, get_action emits
        ABORT before the state exits to SearchStone.
        """
        if self.abort_sent:
            return AIState.SEARCH_STONE
        if self.need_abort:
            return None

        if self.command_sent and context.last_command_successful is not None:
            if context.last_command_successful:
                return AIState.SEARCH_STONE
            evo_cfg = get_evolution_config()
            if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
                self.need_abort = True
                return None
            return AIState.SEARCH_STONE
        return None

    def get_action(self, context: DroneContext) -> str | None:
        """
        Group levels: Broadcast INCANT one tick before Incantation
        (or ABORT if the ritual just failed). Solo levels incant directly.
        """
        if self.need_abort and not self.abort_sent:
            self.abort_sent = True
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.ABORT, context.level, context.drone_id
            )
            return f"Broadcast {payload}"
        evo_cfg = get_evolution_config()
        if not self.broadcast_sent and context.level > evo_cfg.get(
            "SOLO_INCANTATION_LEVEL", 1
        ):
            self.broadcast_sent = True
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.INCANT, context.level, context.drone_id
            )
            return f"Broadcast {payload}"
        if not self.command_sent:
            self.command_sent = True
            return AIState.INCANTATION
        return None

    def exit(self, context: DroneContext) -> None:
        pass
