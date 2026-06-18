from utils.stones import is_incantation_ready, get_missing_stones
from utils.config_loader import (
    get_survival_config,
    get_evolution_config,
    get_reproduction_config,
)
from BroadcastProtocol import MessageType

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from context import DroneContext


class UtilityCalculators:
    """Calculates utilities for various AI behaviors."""

    if TYPE_CHECKING:
        forks_done: int
        context: "DroneContext"
        incant_cmd_sent: bool
        is_leader: bool
        ready_count: int
        is_following: bool
        reproduce_fork_sent: bool
        reproduce_attempted: bool

    def _get_survival_utility(self) -> float:
        surv_cfg = get_survival_config()
        food = self.context.inventory.food
        threshold = surv_cfg.get("SURVIVAL_THRESHOLD", 5)
        target = surv_cfg.get("FOOD_TARGET", 15)
        if food < threshold:
            return 1.0
        if food >= target:
            return 0.0
        return 1.0 - ((food - threshold) / (target - threshold))

    def _get_incantation_utility(self) -> float:
        if not self.context.vision:
            return 0.0
        tile = self.context.vision[0]

        # If we already sent the incantation command, stick to this state until success/fail
        if self.incant_cmd_sent:
            return 1.0

        evo_cfg = get_evolution_config()
        solo_level = evo_cfg.get("SOLO_INCANTATION_LEVEL", 1)

        if self.context.level <= solo_level:
            if is_incantation_ready(self.context.level, tile):
                return 1.0
        else:
            players_req = evo_cfg.get("PLAYERS_REQUIRED", {}).get(
                str(self.context.level), 0
            )
            if (
                self.is_leader
                and self.ready_count + 1 >= players_req
                and is_incantation_ready(self.context.level, tile)
            ):
                return 1.0
        return 0.0

    def _get_gather_utility(self, u_survival: float) -> float:
        missing = get_missing_stones(self.context.level, self.context.inventory)
        if not missing:
            return 0.0
        num_missing = sum(missing.values())
        evo_cfg = get_evolution_config()
        requirements = evo_cfg.get("ELEVATION_REQUIREMENTS", {}).get(
            str(self.context.level), {}
        )
        total_needed = sum(requirements.values()) if requirements else 1

        stone_ratio = num_missing / total_needed
        return 0.8 * stone_ratio * (1.0 - u_survival)

    def _get_reproduce_utility(self, u_survival: float) -> float:
        repr_cfg = get_reproduction_config()
        if self.forks_done >= repr_cfg.get("MAX_FORKS_PER_DRONE", 10):
            return 0.0
        if self.context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            return 0.0

        if self.reproduce_fork_sent:
            return 1.0

        if self.reproduce_attempted:
            return 0.0

        return 0.82 * (1.0 - u_survival)

    def _get_rally_utility(self, u_survival: float) -> float:
        if get_missing_stones(self.context.level, self.context.inventory):
            return 0.0

        if self.is_following:
            return 0.0

        return 0.9 * (1.0 - u_survival)

    def _get_follow_utility(self, u_survival: float) -> float:
        evo_cfg = get_evolution_config()
        if self.context.level <= evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            return 0.0

        if self.is_following:
            return 0.85 * (1.0 - u_survival)

        missing = get_missing_stones(self.context.level, self.context.inventory)
        if not missing:
            # We have all stones, but if someone with a higher ID called, we follow them!
            for bcst in self.context.broadcasts:
                decoded = bcst.content
                if (
                    decoded
                    and decoded.team_name == self.context.team_name
                    and decoded.msg_type == MessageType.RALLY
                    and decoded.level == self.context.level
                ):
                    if decoded.drone_id > self.context.drone_id:
                        return 0.85 * (1.0 - u_survival)
            return 0.0

        for bcst in self.context.broadcasts:
            decoded = bcst.content
            if (
                decoded
                and decoded.team_name == self.context.team_name
                and decoded.msg_type == MessageType.RALLY
                and decoded.level == self.context.level
            ):
                return 0.85 * (1.0 - u_survival)
        return 0.0
