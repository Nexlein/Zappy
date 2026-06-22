##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## The Swarm Layer (Contextual Priority)
##

from fsm.states.AState import AState
from fsm.states.StateNames import AIState
from context import DroneContext
from utils.stones import is_incantation_ready, next_stone_to_drop, next_stone_to_take
from utils.navigation import (
    get_action_for_broadcast,
    BROADCAST_DIRECTION_ARRIVED,
    get_safe_navigation_action,
)
from utils.config_loader import (
    get_survival_config,
    get_swarm_config,
    get_evolution_config,
)
from protocol.BroadcastProtocol import BroadcastProtocol, MessageType


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

        evo_cfg = get_evolution_config()
        if context.vision:
            tile = context.vision[0]
            actual_ready_count = sum(
                1
                for a in context.ally_roster.values()
                if a.level == context.level
                and a.is_ready
                and a.direction in BROADCAST_DIRECTION_ARRIVED
            )
            players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(str(context.level), 0)
            if actual_ready_count + 1 >= players_req and is_incantation_ready(
                context.level, tile
            ):
                return AIState.INCANTATION

        # Yield to another drone with a higher ID if they are calling for the same level
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            for bcst in context.broadcasts:
                if (
                    bcst.content.msg_type == MessageType.RALLY
                    and bcst.content.level == context.level
                    and bcst.content.drone_id > context.drone_id
                ):
                    self._abort_target = AIState.MAPS_TO_ALLY
                    return None

        surv_cfg = get_survival_config()
        swarm_cfg = get_swarm_config()
        if context.inventory.food < surv_cfg.get("SURVIVAL_THRESHOLD", 5):
            self._abort_target = AIState.FORAGE_FOOD
        elif self.ticks_waited > swarm_cfg.get("RALLY_TIMEOUT", 100):
            # Count ALL active allies on the team, not just those of our level.
            # If the team is large enough overall, we just wait for them to catch up.
            active_allies_able_to_help = sum(
                1
                for info in context.ally_roster.values()
                if context.total_ticks - info.last_seen_tick < 1500
                and info.level <= context.level
            )
            players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(str(context.level), 0)
            if active_allies_able_to_help < players_req - 1:
                self._abort_target = AIState.REPRODUCE
            else:
                self.ticks_waited = 0

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
        actual_ready_count = sum(
            1
            for a in context.ally_roster.values()
            if a.level == context.level
            and a.is_ready
            and a.direction in BROADCAST_DIRECTION_ARRIVED
        )
        committed_count = sum(
            1
            for a in context.ally_roster.values()
            if a.level == context.level and (a.is_ready or a.is_coming)
        )

        surv_cfg = get_survival_config()
        safe_food = surv_cfg.get("SAFE_FOOD_THRESHOLD", 10)

        can_start_now = False
        if actual_ready_count + 1 >= players_req:
            if (
                actual_ready_count == committed_count
                or context.inventory.food < safe_food
            ):
                can_start_now = True

        if can_start_now:
            stone_to_take = next_stone_to_take(context.level, context.vision[0])
            if stone_to_take:
                return f"Take {stone_to_take}"

            stone = next_stone_to_drop(
                context.level, context.inventory, context.vision[0]
            )
            if stone:
                return f"Set {stone}"

        # Periodically re-broadcast the RALLY signal (only if higher than solo)
        if context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            msg_type = (
                MessageType.RALLY_FULL
                if committed_count + 1 >= players_req
                else MessageType.RALLY
            )
            swarm_cfg = get_swarm_config()
            if self.tick_since_bcast >= swarm_cfg.get("BCAST_INTERVAL", 2):
                self.tick_since_bcast = 0
                payload = BroadcastProtocol.encode(
                    context.team_name,
                    msg_type,
                    context.level,
                    context.drone_id,
                )
                context.vision.clear()
                return f"Broadcast {payload}"

        self.tick_since_bcast += 1

        # Idle ticks: periodically Look to refresh tile.player count
        if self.tick_since_bcast % 2 == 0:
            context.vision.clear()
            return "Look"

        if context.path_queue:
            return context.path_queue.pop(0)

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

    def __init__(self) -> None:
        self._entry_level = 1
        self.leader_id: str | None = None
        self.arrived = False
        self.tick_since_bcast = 0
        self.ready_sent = False
        self.ticks_waited = 0
        self._last_turn_action: str | None = None
        self._leave_emitted = False
        self._leave_target = None
        self.waiting_incant = False

    def enter(self, context: DroneContext) -> None:
        self._entry_level = context.level
        self.ticks_waited = 0
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self._leave_target = None
        self._leave_emitted = False
        self.tick_since_bcast = 0
        self.leader_id = max(
            (
                id
                for id, info in context.ally_roster.items()
                if info.level == self._entry_level and info.is_rallying
            ),
            default="",
        )

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

        if not getattr(self, "waiting_incant", False):
            self.ticks_waited += 1

        surv_cfg = get_survival_config()
        if context.inventory.food < surv_cfg.get("SURVIVAL_THRESHOLD", 5):
            return self._leave(AIState.FORAGE_FOOD)

        # Leveled up thanks to the elevations
        if context.level > self._entry_level:
            return AIState.SEARCH_STONE

        for bcst in context.broadcasts:
            if bcst.content.level != self._entry_level:
                continue
            if bcst.content.msg_type == MessageType.ABORT:
                return AIState.SEARCH_STONE
            if bcst.content.msg_type == MessageType.INCANT:
                if bcst.direction in BROADCAST_DIRECTION_ARRIVED:
                    self.arrived = True
                    self.waiting_incant = True
                else:
                    return AIState.SEARCH_STONE

        if not self.leader_id:
            return self._leave(AIState.SEARCH_STONE)

        leader_info = context.ally_roster.get(self.leader_id)
        if not leader_info or not leader_info.is_rallying:
            return self._leave(AIState.SEARCH_STONE)

        # Check if we heard the leader recently (e.g. within the timeout window)
        if context.total_ticks - leader_info.last_seen_tick < 100:
            self.ticks_waited = 0

        swarm_cfg = get_swarm_config()
        if self.ticks_waited > swarm_cfg.get("RALLY_TIMEOUT", 100):
            return self._leave(AIState.SEARCH_STONE)

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

        best_direction = None
        if self.leader_id:
            # ONLY act on fresh directional data from the current tick
            for bcst in context.broadcasts:
                if (
                    bcst.content.drone_id == self.leader_id
                    and bcst.content.msg_type
                    in (MessageType.RALLY, MessageType.RALLY_FULL)
                ):
                    best_direction = bcst.direction
                    break

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
                    self.tick_since_bcast += 1
                    if self.tick_since_bcast > 5:
                        self.tick_since_bcast = 0
                        payload = BroadcastProtocol.encode(
                            context.team_name,
                            MessageType.COMING,
                            self._entry_level,
                            context.drone_id,
                        )
                        return f"Broadcast {payload}"

                    action, self._last_turn_action = get_safe_navigation_action(
                        action, self._last_turn_action
                    )
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
