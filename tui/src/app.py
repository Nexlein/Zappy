from pathlib import Path

from textual.app import App, ComposeResult
from textual.containers import Horizontal, Vertical
from textual.widgets import Footer, Header, OptionList

from supervisor.launcher import Launch, launch_profile
from supervisor.profiles import Profile, ProfileError, load_profiles
from supervisor.supervisor import Supervisor
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
    ]

    def __init__(
        self,
        profiles: dict[str, Profile],
        supervisor: Supervisor,
        profiles_path: Path,
    ) -> None:
        super().__init__()
        self._profiles = profiles
        self._supervisor = supervisor
        self._profiles_path = profiles_path
        self._games: list[tuple[str, Launch]] = []

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
        alive = sum(1 for p in self._supervisor.processes if p.is_alive())
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

    def _launch(self, name: str) -> None:
        try:
            launch = launch_profile(self._supervisor, self._profiles[name])
        except OSError as e:
            self.notify(f"launch failed: {e}", severity="error")
            return
        self._games.append((name, launch))
        self.notify(f"launched '{name}' on port {launch.port}")
        self._refresh_processes()

    def _tick(self) -> None:
        self._supervisor.reap()
        self._refresh_processes()
        self.query_one(LogPanel).poll()

    def _refresh_processes(self) -> None:
        self.query_one(ProcessList).show(self._games)
