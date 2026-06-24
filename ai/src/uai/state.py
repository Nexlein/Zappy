from dataclasses import dataclass


@dataclass
class UAIState:
    # Generic
    forward_streak: int = 0
    tick_since_bcast: int = 0
    last_level: int = 1
    last_behavior: str = "survival"

    # Leader
    ready_count: int = 0
    is_leader: bool = False
    leader_aborted: bool = False
    rally_ticks: int = 0
    incant_bcast_sent: bool = False
    incant_cmd_sent: bool = False

    # Follower
    is_following: bool = False
    arrived: bool = False
    ready_sent: bool = False
    waiting_incant: bool = False
    follower_leaving: bool = False
    target_leader_id: str = ""
    highest_rally_direction: int | None = None

    # Reproduce
    forks_done: int = 0
    reproduce_connect_sent: bool = False
    reproduce_fork_sent: bool = False
    reproduce_spawn_sent: bool = False
    last_fork_tick: int = -9999

    def reset_leader_state(self) -> None:
        self.ready_count = 0
        self.is_leader = False
        self.leader_aborted = False
        self.rally_ticks = 0
        self.incant_bcast_sent = False
        self.incant_cmd_sent = False

    def reset_follower_state(self) -> None:
        self.is_following = False
        self.arrived = False
        self.ready_sent = False
        self.waiting_incant = False
        self.follower_leaving = False
        self.target_leader_id = ""
        self.highest_rally_direction = None

    def reset_reproduce_state(self) -> None:
        self.reproduce_connect_sent = False
        self.reproduce_fork_sent = False
        self.reproduce_spawn_sent = False
