from pathlib import Path

from textual.app import App, ComposeResult
from textual.containers import Horizontal, Vertical
from textual.widgets import Footer, Header, OptionList

from supervisor.manager import Game, ZappyManager
from supervisor.process import ManagedProcess
from supervisor.profiles import Profile, ProfileError, load_profiles
from supervisor.supervisor import Supervisor
from widgets.attach_screen import AttachAiScreen
from widgets.detail_panel import DetailPanel
from widgets.log_panel import LogPanel
from widgets.process_list import ProcessList
from widgets.profile_list import ProfileList
from widgets.quit_screen import QuitScreen


class ZappyTUI(App):
    TITLE = "Zappy orchestrator"
    CSS_PATH = "app.tcss"
    BINDINGS = [
        ("q", "request_quit", "Quit"),
        ("r", "reload", "Reload profiles"),
        ("a", "attach_ai", "Attach AI"),
        ("g", "attach_gui", "Attach GUI"),
        ("k", "kill_process", "Kill"),
        ("s", "stop_game", "Stop game"),
    ]

    def __init__(
        self,
        profiles: dict[str, Profile],
        supervisor: Supervisor,
        profiles_path: Path,
    ) -> None:
        super().__init__()
        self._profiles = profiles
        self._profiles_path = profiles_path
        self._manager = ZappyManager(supervisor)

    def compose(self) -> ComposeResult:
        yield Header()
        with Horizontal():
            with Vertical(id="sidebar"):
                yield ProfileList(self._profiles)
                yield ProcessList()
            with Vertical(id="main"):
                yield DetailPanel()
                yield LogPanel()
        yield Footer()

    def on_mount(self) -> None:
        # poll children: reap the dead, refresh the list, update labels
        self.set_interval(1.0, self._tick)

    def on_option_list_option_selected(self, event: OptionList.OptionSelected) -> None:
        if isinstance(event.option_list, ProfileList) and event.option.id is not None:
            self._launch(event.option.id)

    def on_option_list_option_highlighted(
        self, event: OptionList.OptionHighlighted
    ) -> None:
        panel = self.query_one(DetailPanel)
        logs = self.query_one(LogPanel)
        if isinstance(event.option_list, ProfileList) and event.option.id is not None:
            panel.show_profile(self._profiles[event.option.id])
            logs.clear_follow()
        elif isinstance(event.option_list, ProcessList):
            process = event.option_list.process_at(event.option_index)
            if process is not None:
                panel.show_process(process)
                logs.follow(process)

    def action_request_quit(self) -> None:
        """Quit immediately if nothing is running, else confirm via a popup."""
        if isinstance(self.screen, QuitScreen):
            return  # dialog already open
        alive = sum(1 for p in self._manager.processes if p.is_alive())
        if alive == 0:
            self.exit()
            return
        self.push_screen(QuitScreen(alive), self._quit_decided)

    def _quit_decided(self, confirm: bool | None) -> None:
        if confirm:
            self.exit()

    def action_reload(self) -> None:
        try:
            profiles = load_profiles(self._profiles_path)
        except ProfileError as e:
            self.notify(f"reload failed: {e}", severity="error")
            return
        self._profiles = profiles
        self.query_one(ProfileList).set_profiles(profiles)
        self.notify(f"reloaded {len(profiles)} profile(s)")

    def action_kill_process(self) -> None:
        """Stop the highlighted child only; the rest of its game keeps running."""
        process = self._highlighted_process()
        if process is None:
            return
        self._manager.kill(process)
        self.notify(f"killed {process.name}")
        self._refresh_processes()

    def action_stop_game(self) -> None:
        """Stop the highlighted process's whole game: server, AIs and GUIs."""
        game = self._manager.game_of(self._highlighted_process())
        if game is None:
            return
        self._manager.stop_game(game)
        self.notify(f"stopped game '{game.name}'")
        self._refresh_processes()

    def action_attach_ai(self) -> None:
        """Pick a team and algorithm, then spawn one more AI on the game."""
        process = self._highlighted_process()
        game = self._manager.game_of(process)
        if game is None:
            return
        teams = self._manager.team_names(game)
        default_team = self._manager.default_team(game, near=process)
        if default_team is None:
            self.notify("no team to attach to", severity="error")
            return
        strategy = self._manager.strategy_of(game, default_team)
        screen = AttachAiScreen(teams, default_team, strategy)
        self.push_screen(screen, lambda choice: self._attach_ai_chosen(game, choice))

    def _attach_ai_chosen(
        self, game: Game, choice: tuple[str, str | None] | None
    ) -> None:
        if choice is None:
            return  # cancelled
        team, strategy = choice
        try:
            ai = self._manager.attach_ai(game, team, strategy)
        except OSError as e:
            self.notify(f"attach failed: {e}", severity="error")
            return
        self.notify(f"attached {ai.name}")
        self._refresh_processes()

    def action_attach_gui(self) -> None:
        """Spawn one more GUI on the highlighted game; GUIs stack freely."""
        game = self._manager.game_of(self._highlighted_process())
        if game is None:
            return
        try:
            gui = self._manager.attach_gui(game)
        except OSError as e:
            self.notify(f"attach failed: {e}", severity="error")
            return
        self.notify(f"attached {gui.name} to '{game.name}'")
        self._refresh_processes()

    def _highlighted_process(self) -> ManagedProcess | None:
        plist = self.query_one(ProcessList)
        if plist.highlighted is None:
            return None
        return plist.process_at(plist.highlighted)

    def _launch(self, name: str) -> None:
        try:
            game = self._manager.launch(name, self._profiles[name])
        except OSError as e:
            self.notify(f"launch failed: {e}", severity="error")
            return
        self.notify(f"launched '{name}' on port {game.launch.port}")
        self._refresh_processes()

    def _tick(self) -> None:
        self._manager.reap()
        self._refresh_processes()
        self.query_one(LogPanel).poll()

    def _refresh_processes(self) -> None:
        self.query_one(ProcessList).show(self._manager.games)
