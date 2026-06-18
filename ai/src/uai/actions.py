import random
from utils.stones import next_stone_to_drop, next_stone_to_take, get_missing_stones
from utils.navigation import get_action_for_broadcast
from utils.config_loader import get_evolution_config, get_swarm_config
from BroadcastProtocol import BroadcastProtocol, MessageType
from look_parser import generate_path_to_tile

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from context import DroneContext


class ActionGenerators:
    """Generates commands based on the selected AI behavior."""

    if TYPE_CHECKING:
        context: "DroneContext"
        _forward_streak: int
        incant_cmd_sent: bool
        leader_aborted: bool
        incant_bcast_sent: bool
        tick_since_bcast: int
        arrived: bool
        ready_sent: bool
        ready_count: int
        highest_rally_direction: int | None
        forks_done: int
        reproduce_connect_sent: bool
        reproduce_fork_sent: bool

        def tick(self) -> str | None: ...
        def _reset_reproduce_state(self) -> None: ...

    def _get_survival_action(self) -> str:
        if self.context.path_queue:
            return self.context.path_queue.pop(0)
        if not self.context.vision:
            return "Look"
        if self.context.vision[0].food > 0:
            self._forward_streak = 0
            return "Take food"
        best_path = None
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            if tile.food > 0:
                path = generate_path_to_tile(i)
                if best_path is None or len(path) < len(best_path):
                    best_path = path

        if best_path:
            self._forward_streak = 0
            self.context.path_queue.extend(best_path)
            return self.context.path_queue.pop(0)
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_gather_action(self) -> str:
        if self.context.path_queue:
            return self.context.path_queue.pop(0)
        if not self.context.vision:
            return "Look"
        missing = get_missing_stones(self.context.level, self.context.inventory)
        current_tile = self.context.vision[0]
        for stone in missing:
            if getattr(current_tile, stone, 0) > 0:
                self._forward_streak = 0
                return f"Take {stone}"
        best_path = None
        for i, tile in enumerate(self.context.vision):
            if i == 0:
                continue
            for stone in missing:
                if getattr(tile, stone, 0) > 0:
                    path = generate_path_to_tile(i)
                    if best_path is None or len(path) < len(best_path):
                        best_path = path
                    break

        if best_path:
            self._forward_streak = 0
            self.context.path_queue.extend(best_path)
            return self.context.path_queue.pop(0)
        self._forward_streak += 1
        if self._forward_streak % 5 == 0:
            return random.choice(["Right", "Left"])
        return "Forward"

    def _get_reproduce_action(self) -> str | None:
        if self.reproduce_fork_sent:
            if self.context.last_command_successful:
                self.forks_done += 1
            self._reset_reproduce_state()
            return None

        if not self.reproduce_connect_sent:
            self.reproduce_connect_sent = True
            return "Connect_nbr"

        if self.context.available_slots == 0:
            self.reproduce_fork_sent = True
            return "Fork"

        self._reset_reproduce_state()
        return None

    def _get_incantation_action(self) -> str | None:
        # Check if previous incantation failed
        if self.incant_cmd_sent and self.context.last_command_successful is False:
            self.leader_aborted = True
            return self.tick()  # trigger abort

        evo_cfg = get_evolution_config()
        if not self.incant_bcast_sent and self.context.level > evo_cfg.get(
            "SOLO_INCANTATION_LEVEL", 1
        ):
            self.incant_bcast_sent = True
            payload = BroadcastProtocol.encode(
                self.context.team_name,
                MessageType.INCANT,
                self.context.level,
                self.context.drone_id,
            )
            return f"Broadcast {payload}"

        if not self.incant_cmd_sent:
            self.incant_cmd_sent = True
            return "Incantation"
        return None

    def _get_rally_action(self) -> str | None:
        if not self.context.vision:
            return "Look"

        evo_cfg = get_evolution_config()
        players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(
            str(self.context.level), 0
        )
        if self.ready_count + 1 >= players_req:
            stone_to_take = next_stone_to_take(
                self.context.level, self.context.vision[0]
            )
            if stone_to_take:
                return f"Take {stone_to_take}"

            stone = next_stone_to_drop(
                self.context.level, self.context.inventory, self.context.vision[0]
            )
            if stone:
                return f"Set {stone}"

        if self.context.level > evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            swarm_cfg = get_swarm_config()
            if self.tick_since_bcast >= swarm_cfg.get("BCAST_INTERVAL", 2):
                self.tick_since_bcast = 0
                payload = BroadcastProtocol.encode(
                    self.context.team_name,
                    MessageType.RALLY,
                    self.context.level,
                    self.context.drone_id,
                )
                return f"Broadcast {payload}"

        self.tick_since_bcast += 1

        if self.ready_count + 1 >= players_req or self.tick_since_bcast % 3 == 0:
            return "Look"
        return None

    def _get_follow_action(self) -> str | None:
        direction = self.highest_rally_direction

        if direction is not None:
            if direction != 0 and self.arrived:
                # Leader moved or changed
                self.arrived = False
                self.ready_sent = False

            if not self.arrived:
                action = get_action_for_broadcast(direction)
                if action is None:
                    self.arrived = True
                else:
                    return action

        if self.arrived:
            if not self.ready_sent:
                self.ready_sent = True
                payload = BroadcastProtocol.encode(
                    self.context.team_name,
                    MessageType.READY,
                    self.context.level,
                    self.context.drone_id,
                )
                return f"Broadcast {payload}"
            return None

        return None
