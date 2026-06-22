##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Swarm Layer (Contextual Priority)
##

from fsm.states.AState import AState
from context import DroneContext
from utils.stones import is_incantation_ready, next_stone_to_drop, next_stone_to_take
from utils.navigation import get_action_for_broadcast, BROADCAST_DIRECTION_ARRIVED
from utils.config_loader import (
    get_survival_config,
    get_swarm_config,
    get_evolution_config,
)
from BroadcastProtocol import BroadcastProtocol, MessageType


def other_players_on_tile(context: DroneContext, tile) -> int:
    """
    Players on `tile` other than us.

    Look returns an anonymous `player` token (no team, no level), so we cannot
    tell allies from enemies. The defensive eject only runs from BroadcastHelp
    before the first RALLY is sent — at that point no follower has been summoned
    yet, so any stranger here can only be an outsider, never our own swarm.

    Solo levels are skipped: spawn clustering would make eject noisy and an
    extra body never changes a solo incantation.
    """
    evo_cfg = get_evolution_config()
    if context.level <= evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
        return 0
    return max(0, tile.player - 1)


# BroadcastHelp: Yell across the map and wait for allies to arrive.
class BroadcastHelp(AState):
    """
    Swarm state — Contextual Priority:
      1. Drop required stones on the current tile.
      2. Broadcast RALLY to call teammates.
      3. Trigger the incantation once the tile is ready.
    """

    def enter(self, context: DroneContext) -> None:
        self.ticks_waited = 0
        swarm_cfg = get_swarm_config()
        self.tick_since_bcast = swarm_cfg.get(
            "BCAST_INTERVAL", 2
        )  # broadcast on the first tick
        self.ready_count = 0
        self._abort_target = None
        self._abort_emitted = False
        self._eject_done = False

    def update(self, context: DroneContext) -> str | None:
        """
        Count READY (K=0) / LEAVING confirmations; launch the incantation once
        enough teammates confirmed AND the tile passes is_incantation_ready.
        On hunger/timeout, get_action emits ABORT before the state exits.
        """
        if self._abort_target:
            return self._abort_target if self._abort_emitted else None

        self.ticks_waited += 1

        for bcst in context.broadcasts:
            if bcst.content.level != context.level:
                continue
            if (
                bcst.content.msg_type == MessageType.READY
                and bcst.direction in BROADCAST_DIRECTION_ARRIVED
            ):
                self.ready_count += 1
            elif bcst.content.msg_type == MessageType.LEAVING and self.ready_count > 0:
                self.ready_count -= 1

        evo_cfg = get_evolution_config()
        if context.vision:
            tile = context.vision[0]
            players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(str(context.level), 0)
            if self.ready_count + 1 >= players_req and is_incantation_ready(
                context.level, tile
            ):
                return "Incantation"

        # Yield to another drone with a higher ID if they are calling for the same level
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            for bcst in context.broadcasts:
                if (
                    bcst.content.msg_type == MessageType.RALLY
                    and bcst.content.level == context.level
                    and bcst.content.drone_id > context.drone_id
                ):
                    self._abort_target = "MapsToAlly"
                    return None

        surv_cfg = get_survival_config()
        swarm_cfg = get_swarm_config()
        if context.inventory.food < surv_cfg.get("SURVIVAL_THRESHOLD", 5):
            self._abort_target = "ForageFood"
        elif self.ticks_waited > swarm_cfg.get("RALLY_TIMEOUT", 100):
            self._abort_target = "Reproduce"

        return None

    def get_action(self, context: DroneContext) -> str | None:
        if self._abort_target and not self._abort_emitted:
            self._abort_emitted = True
            payload = BroadcastProtocol.encode(
                context.team_name, MessageType.ABORT, context.level, context.drone_id
            )
            return f"Broadcast {payload}"

        if not context.vision:
            return "Look"

        if not self._eject_done and (
            other_players_on_tile(context, context.vision[0]) > 0
        ):
            self._eject_done = True
            return "Eject"

        evo_cfg = get_evolution_config()
        players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(str(context.level), 0)
        if self.ready_count + 1 >= players_req:
            stone_to_take = next_stone_to_take(context.level, context.vision[0])
            if stone_to_take:
                return f"Take {stone_to_take}"

            stone = next_stone_to_drop(
                context.level, context.inventory, context.vision[0]
            )
            if stone:
                return f"Set {stone}"

        # Periodically re-broadcast the RALLY signal (only if higher than solo).
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            swarm_cfg = get_swarm_config()
            if self.tick_since_bcast >= swarm_cfg.get("BCAST_INTERVAL", 2):
                self.tick_since_bcast = 0
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    MessageType.RALLY,
                    context.level,
                    context.drone_id,
                )
                return f"Broadcast {payload}"

        self.tick_since_bcast += 1
        return "Look"

    def exit(self, context: DroneContext) -> None:
        pass


# MapsToAlly: Navigate toward a teammate's broadcast signal.
class MapsToAlly(AState):
    """
    Swarm state — Contextual Priority:
      Navigate toward the ally broadcasting RALLY by reading the direction K
      from each incoming broadcast message.
    """

    def enter(self, context: DroneContext) -> None:
        self._entry_level = context.level
        self.ticks_waited = 0
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self._leave_target = None
        self._leave_emitted = False
        self._target_leader_id = ""

    def _leave(self, target: str) -> str | None:
        """Exit toward `target`, emitting LEAVING first if we said READY."""
        if not self.ready_sent:
            return target
        self._leave_target = target
        return None

    def update(self, context: DroneContext) -> str | None:
        """
        Exit on hunger/level-up/timeout (emitting LEAVING first if READY was
        sent) and react to the initiator's INCANT (K=0: hold still, else too
        late) and ABORT signals.
        """
        if self._leave_target:
            return self._leave_target if self._leave_emitted else None

        self.ticks_waited += 1

        surv_cfg = get_survival_config()
        if context.inventory.food < surv_cfg.get("SURVIVAL_THRESHOLD", 5):
            return self._leave("ForageFood")

        # Leveled up thanks to the elevations
        if context.level > self._entry_level:
            return "SearchStone"

        for bcst in context.broadcasts:
            if bcst.content.level != self._entry_level:
                continue
            if bcst.content.msg_type == MessageType.ABORT:
                return "SearchStone"
            if bcst.content.msg_type == MessageType.INCANT:
                if bcst.direction in BROADCAST_DIRECTION_ARRIVED:
                    self.arrived = True
                    self.waiting_incant = True
                else:
                    return "SearchStone"

        # Also switch to the highest known leader in update so we don't ignore ABORTs from them.
        heard_leader = False
        for bcst in context.broadcasts:
            if (
                bcst.content.msg_type == MessageType.RALLY
                and bcst.content.level == self._entry_level
            ):
                if bcst.content.drone_id > self._target_leader_id:
                    self._target_leader_id = bcst.content.drone_id
                if bcst.content.drone_id == self._target_leader_id:
                    heard_leader = True

        if heard_leader:
            self.ticks_waited = 0

        swarm_cfg = get_swarm_config()
        if self.ticks_waited > swarm_cfg.get("RALLY_TIMEOUT", 100):
            return self._leave("SearchStone")

        return None

    def get_action(self, context: DroneContext) -> str | None:
        """
        Emit a pending LEAVING, stay silent if the ritual is imminent,
        drop stones then say READY once on the rally tile, or follow
        the RALLY direction while travelling.
        """
        if self._leave_target and not self._leave_emitted:
            self._leave_emitted = True
            payload = BroadcastProtocol.encode(
                context.team_name,
                MessageType.LEAVING,
                self._entry_level,
                context.drone_id,
            )
            return f"Broadcast {payload}"

        if self.waiting_incant:
            return None

        # -- Process new broadcasts first to track the highest leader --
        best_direction = None
        for bcst in context.broadcasts:
            if (
                bcst.content.msg_type == MessageType.RALLY
                and bcst.content.level == self._entry_level
                and bcst.content.drone_id >= self._target_leader_id
            ):
                self._target_leader_id = bcst.content.drone_id
                best_direction = bcst.direction

        if best_direction is not None:
            if best_direction != 0 and self.arrived:
                # The highest leader is not here! We must have arrived at an abandoned tile.
                self.arrived = False
                self.ready_sent = False

            if not self.arrived:
                action = get_action_for_broadcast(best_direction)
                if action is None:
                    self.arrived = True
                else:
                    return action

        # -- Already on the rally tile (or just arrived): wait for the leader. --
        if self.arrived:
            if not self.ready_sent:
                self.ready_sent = True
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    MessageType.READY,
                    self._entry_level,
                    context.drone_id,
                )
                return f"Broadcast {payload}"
            return "Look"  # Wait for INCANT

        # No RALLY heard this tick and not arrived — stay put and wait for the next broadcast.
        return "Look"

    def exit(self, context: DroneContext) -> None:
        pass
