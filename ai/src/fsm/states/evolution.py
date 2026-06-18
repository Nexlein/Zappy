##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Evolution Layer (Medium Priority)
##

import random
from utils.config_loader import get_survival_config, get_evolution_config
from utils.stones import get_missing_stones
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType
from context import DroneContext
from protocol.look_parser import find_closest_resource_path

from fsm.states.AState import AState


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

    def _get_missing_stones_for_drone(self, context: DroneContext) -> dict[str, int]:
        return get_missing_stones(context.level, context.inventory)

    def update(self, context: DroneContext) -> str | None:
        evo_cfg = get_evolution_config()
        surv_cfg = get_survival_config()

        if context.level >= evo_cfg.get("MAX_LEVEL", 8):
            return "ForageFood"

        if context.inventory.food < surv_cfg.get("SAFE_FOOD_THRESHOLD", 10):
            return "ForageFood"

        # React to a teammate's RALLY call (solo levels can incant alone).
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            for bcst in context.broadcasts:
                if (
                    bcst.content.msg_type in (MessageType.RALLY, MessageType.RALLY_FULL)
                    and bcst.content.level == context.level
                ):
                    return "MapsToAlly"

        missing = self._get_missing_stones_for_drone(context)
        if not missing:
            # Force the leader to be full food
            if context.inventory.food < surv_cfg.get("FOOD_TARGET", 25):
                return "ForageFood"
            return "BroadcastHelp"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if context.path_queue:
            return context.path_queue.pop(0)

        if not context.vision:
            return "Look"

        missing_stones = self._get_missing_stones_for_drone(context)
        current_tile = context.vision[0]

        # Pick up any needed stone on the current tile, unless there are other players here (could be a ritual).
        if current_tile.player <= 1:
            for stone in missing_stones:
                if getattr(current_tile, stone, 0) > 0:
                    self._forward_streak = 0
                    return f"Take {stone}"

        best_path = find_closest_resource_path(context.vision, missing_stones)

        if best_path:
            self._forward_streak = 0
            context.path_queue.extend(best_path)
            return context.path_queue.pop(0)

        # Nothing useful here — explore.
        # Rotate every 5 steps to avoid getting stuck in a straight line.
        self._forward_streak += 1
        surv_cfg = get_survival_config()
        explore_turn_every = surv_cfg.get("EXPLORE_TURN_EVERY", 5)
        if self._forward_streak % explore_turn_every == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

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
            return "SearchStone"
        if self.need_abort:
            return None

        if self.command_sent and context.last_command_successful is not None:
            if context.last_command_successful:
                return "SearchStone"
            evo_cfg = get_evolution_config()
            if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
                self.need_abort = True
                return None
            return "SearchStone"
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
            return "Incantation"
        return None

    def exit(self, context: DroneContext) -> None:
        pass
