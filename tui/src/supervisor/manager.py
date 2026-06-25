from __future__ import annotations

from dataclasses import dataclass

from .commands import Binaries
from .launcher import Launch, attach_ai, attach_gui, launch_profile
from .process import ManagedProcess
from .profiles import Profile
from .supervisor import Supervisor


@dataclass
class Game:
    """A launched profile: its display name, the profile it booted from (kept so
    later attaches resolve teams/strategies even after a reload) and the live
    process handle."""

    name: str
    profile: Profile
    launch: Launch


class ZappyManager:
    """All running games on top of a Supervisor. Pure logic, no UI: the app
    drives it and renders ``games``. Each control acts on a Game (or a single
    process within one) without the UI knowing the spawn details."""

    def __init__(
        self,
        supervisor: Supervisor,
        binaries: Binaries = Binaries(),
        host: str | None = None,
    ) -> None:
        self._supervisor = supervisor
        self._binaries = binaries
        self._host = host
        self._games: list[Game] = []

    @property
    def games(self) -> list[Game]:
        return list(self._games)

    @property
    def processes(self) -> list[ManagedProcess]:
        return self._supervisor.processes

    def launch(self, name: str, profile: Profile) -> Game:
        launch = launch_profile(self._supervisor, profile, self._binaries, self._host)
        game = Game(name, profile, launch)
        self._games.append(game)
        return game

    def reap(self) -> None:
        self._supervisor.reap()

    def kill(self, process: ManagedProcess) -> None:
        """Stop one child; the rest of its game keeps running."""
        self._supervisor.stop(process)

    def stop_game(self, game: Game) -> None:
        """Stop a game's server, AIs and GUIs."""
        launch = game.launch
        for member in (launch.server, *launch.ais, *launch.guis):
            self._supervisor.stop(member)

    def attach_ai(
        self, game: Game, team: str, strategy: str | None = None
    ) -> ManagedProcess:
        """Spawn one more AI for ``team`` with the given strategy (None = the
        AI's default)."""
        return attach_ai(self._supervisor, game.launch, team, strategy)

    def attach_gui(self, game: Game) -> ManagedProcess:
        """Spawn one more GUI; GUIs stack freely."""
        return attach_gui(self._supervisor, game.launch)

    def team_names(self, game: Game) -> list[str]:
        return [team.name for team in game.profile.teams]

    def default_team(
        self, game: Game, near: ManagedProcess | None = None
    ) -> str | None:
        """Best team to preselect: the one ``near`` belongs to if it is an AI,
        else the profile's first team (None if the profile has no teams)."""
        return self._team_for(game, near)

    def strategy_of(self, game: Game, team: str) -> str | None:
        return next((t.strategy for t in game.profile.teams if t.name == team), None)

    def game_of(self, process: ManagedProcess | None) -> Game | None:
        if process is None:
            return None
        for game in self._games:
            launch = game.launch
            if (
                process is launch.server
                or process in launch.ais
                or process in launch.guis
            ):
                return game
        return None

    @staticmethod
    def _team_for(game: Game, near: ManagedProcess | None) -> str | None:
        if near is not None and near in game.launch.ais:
            return near.name.rsplit("-", 1)[0]
        return game.profile.teams[0].name if game.profile.teams else None
