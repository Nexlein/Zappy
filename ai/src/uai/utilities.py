from elevations import ELEVATION_REQUIREMENTS, PLAYERS_REQUIRED, is_incantation_ready
from config import (
    FOOD_TARGET,
    SURVIVAL_THRESHOLD,
    SOLO_INCANTATION_LEVEL,
    FORK_FOOD_THRESHOLD,
    MAX_FORKS_PER_DRONE,
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

        def _get_missing_stones(self) -> dict[str, int]: ...

    def _get_survival_utility(self) -> float:
        food = self.context.inventory.food
        if food < SURVIVAL_THRESHOLD:
            return 1.0
        if food >= FOOD_TARGET:
            return 0.0
        return 1.0 - ((food - SURVIVAL_THRESHOLD) / (FOOD_TARGET - SURVIVAL_THRESHOLD))

    def _get_incantation_utility(self) -> float:
        if not self.context.vision:
            return 0.0
        tile = self.context.vision[0]

        # If we already sent the incantation command, stick to this state until success/fail
        if self.incant_cmd_sent:
            return 1.0

        if self.context.level <= SOLO_INCANTATION_LEVEL:
            if is_incantation_ready(self.context.level, tile):
                return 1.0
        else:
            if (
                self.is_leader
                and self.ready_count + 1 >= PLAYERS_REQUIRED.get(self.context.level, 0)
                and is_incantation_ready(self.context.level, tile)
            ):
                return 1.0
        return 0.0

    def _get_gather_utility(self, u_survival: float) -> float:
        missing = self._get_missing_stones()
        if not missing:
            return 0.0
        num_missing = sum(missing.values())
        requirements = ELEVATION_REQUIREMENTS.get(self.context.level, {})
        total_needed = sum(requirements.values()) if requirements else 1

        stone_ratio = num_missing / total_needed
        return 0.8 * stone_ratio * (1.0 - u_survival)

    def _get_reproduce_utility(self, u_survival: float) -> float:
        if self.forks_done >= MAX_FORKS_PER_DRONE:
            return 0.0
        if self.context.inventory.food < FORK_FOOD_THRESHOLD:
            return 0.0

        if self.reproduce_fork_sent:
            return 1.0

        if self.reproduce_attempted:
            return 0.0

        return 0.82 * (1.0 - u_survival)

    def _get_rally_utility(self, u_survival: float) -> float:
        if self._get_missing_stones():
            return 0.0

        if self.is_following:
            return 0.0

        return 0.9 * (1.0 - u_survival)

    def _get_follow_utility(self, u_survival: float) -> float:
        if self.context.level <= SOLO_INCANTATION_LEVEL:
            return 0.0

        if self.is_following:
            return 0.85 * (1.0 - u_survival)

        missing = self._get_missing_stones()
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
