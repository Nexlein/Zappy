from context import DroneContext
from uai.state import UAIState
from utils.stones import is_incantation_ready, get_missing_stones
from utils.config_loader import (
    get_survival_config,
    get_evolution_config,
    get_reproduction_config,
)
from protocol.BroadcastProtocol import MessageType


class UtilityCalculator:
    """Calculates utilities for various AI behaviors."""

    def __init__(self, context: DroneContext, state: UAIState):
        self.context = context
        self.state = state

    def get_survival_utility(self) -> float:
        surv_cfg = get_survival_config()
        food = self.context.inventory.food
        threshold = surv_cfg.get("SURVIVAL_THRESHOLD", 5)
        target = surv_cfg.get("FOOD_TARGET", 15)
        if food < threshold:
            return 1.0
        if food >= target:
            return 0.0
        return 1.0 - ((food - threshold) / (target - threshold))

    def get_incantation_utility(self) -> float:
        if not self.context.vision:
            return 0.0
        tile = self.context.vision[0]

        if self.state.incant_cmd_sent:
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
                self.state.is_leader
                and self.state.ready_count + 1 >= players_req
                and is_incantation_ready(self.context.level, tile)
            ):
                return 1.0
        return 0.0

    def get_gather_utility(self, u_survival: float) -> float:
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

    def get_reproduce_utility(self, u_survival: float) -> float:
        repr_cfg = get_reproduction_config()
        if self.state.forks_done >= repr_cfg.get("MAX_FORKS_PER_DRONE", 10):
            return 0.0
        if self.context.inventory.food < repr_cfg.get("FORK_FOOD_THRESHOLD", 10):
            return 0.0

        if self.context.total_ticks - self.state.last_fork_tick < 600:
            return 0.0

        if (
            self.state.reproduce_fork_sent
            or self.state.reproduce_spawn_sent
            or self.state.reproduce_connect_sent
        ):
            return 1.0

        return 0.0

    def get_rally_utility(self, u_survival: float) -> float:
        if get_missing_stones(self.context.level, self.context.inventory):
            return 0.0

        if self.state.is_following:
            return 0.0

        return 0.9 * (1.0 - u_survival)

    def get_follow_utility(self, u_survival: float) -> float:
        evo_cfg = get_evolution_config()
        if self.context.level <= evo_cfg.get("SOLO_INCANTATION_LEVEL", 1):
            return 0.0

        if self.state.is_following:
            return 0.85 * (1.0 - u_survival)

        missing = get_missing_stones(self.context.level, self.context.inventory)
        if not missing:
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

    def calculate_all(self) -> dict[str, float]:
        u_survival = self.get_survival_utility()
        return {
            "survival": u_survival,
            "incantation": self.get_incantation_utility(),
            "rally": self.get_rally_utility(u_survival),
            "follow": self.get_follow_utility(u_survival),
            "gather": self.get_gather_utility(u_survival),
            "reproduce": self.get_reproduce_utility(u_survival),
        }
