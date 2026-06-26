from textual.widgets import Static

from supervisor.manager import Game
from supervisor.process import ManagedProcess
from supervisor.profiles import Profile


def _fmt_uptime(seconds: float | None) -> str:
    """``HH:MM:SS`` from a second count; ``-`` if the process never started."""
    if seconds is None:
        return "-"
    total = int(seconds)
    return f"{total // 3600:02d}:{total % 3600 // 60:02d}:{total % 60:02d}"


class DetailPanel(Static):
    """Right pane: details of whatever is highlighted — a profile's config, a
    whole game's live status, or a single process. The app feeds it on
    selection changes and re-feeds it each tick so uptime/state stay live."""

    def on_mount(self) -> None:
        self.border_title = "Details"

    def show_profile(self, profile: Profile) -> None:
        teams = "\n".join(
            f"  - {t.name}: {t.ai} ai ({t.strategy or 'default'})"
            for t in profile.teams
        )
        self.update(
            f"profile: {profile.name}\n"
            f"map:     {profile.width}x{profile.height}\n"
            f"clients: {profile.clients}\n"
            f"freq:    {profile.freq}\n"
            f"auto_gui: {profile.auto_gui}\n"
            f"teams:\n{teams}"
        )

    def show_game(self, game: Game) -> None:
        launch = game.launch
        srv = launch.server
        srv_state = "alive" if srv.is_alive() else "dead"
        alive_ais = sum(1 for ai in launch.ais if ai.is_alive())
        alive_guis = sum(1 for gui in launch.guis if gui.is_alive())
        self.update(
            f"game:    {game.name}\n"
            f"server:  port {launch.port}  ({srv_state})\n"
            f"uptime:  {_fmt_uptime(srv.uptime)}\n"
            f"players: {alive_ais} alive / {len(launch.ais)} spawned\n"
            f"guis:    {alive_guis} alive / {len(launch.guis)} spawned\n"
            f"ais:\n{self._ais_by_team(game)}"
        )

    @staticmethod
    def _ais_by_team(game: Game) -> str:
        lines: list[str] = []
        for team in game.profile.teams:
            members = [
                ai for ai in game.launch.ais if ai.name.rsplit("-", 1)[0] == team.name
            ]
            lines.append(f"  {team.name}:")
            if not members:
                lines.append("    (none)")
            for ai in members:
                state = "alive" if ai.is_alive() else "dead"
                lines.append(f"    - {ai.name}  {state}")
        return "\n".join(lines)

    def show_process(self, process: ManagedProcess) -> None:
        state = "alive" if process.is_alive() else "dead"
        self.update(
            f"process: {process.name}\n"
            f"pid:     {process.pid}\n"
            f"state:   {state}\n"
            f"uptime:  {_fmt_uptime(process.uptime)}\n"
            f"rc:      {process.returncode}\n"
            f"command:\n  {' '.join(process.command)}"
        )
