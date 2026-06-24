from textual.app import App, ComposeResult
from textual.containers import Horizontal, Vertical
from textual.widgets import Footer, Header, OptionList

from supervisor.launcher import launch_profile
from supervisor.profiles import Profile
from supervisor.supervisor import Supervisor
from widgets.detail_panel import DetailPanel
from widgets.process_list import ProcessList
from widgets.profile_list import ProfileList


class ZappyTUI(App):
    TITLE = "Zappy orchestrator"
    CSS_PATH = "app.tcss"
    BINDINGS = [("q", "quit", "Quit")]

    def __init__(self, profiles: dict[str, Profile], supervisor: Supervisor) -> None:
        super().__init__()
        self._profiles = profiles
        self._supervisor = supervisor

    def compose(self) -> ComposeResult:
        yield Header()
        with Horizontal():
            with Vertical(id="sidebar"):
                yield ProfileList(self._profiles)
                yield ProcessList()
            yield DetailPanel()
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
        if isinstance(event.option_list, ProfileList) and event.option.id is not None:
            panel.show_profile(self._profiles[event.option.id])
        elif isinstance(event.option_list, ProcessList):
            processes = self._supervisor.processes
            if 0 <= event.option_index < len(processes):
                panel.show_process(processes[event.option_index])

    def _launch(self, name: str) -> None:
        try:
            launch_profile(self._supervisor, self._profiles[name])
        except OSError as e:
            self.notify(f"launch failed: {e}", severity="error")
            return
        self._refresh_processes()

    def _tick(self) -> None:
        self._supervisor.reap()
        self._refresh_processes()

    def _refresh_processes(self) -> None:
        self.query_one(ProcessList).show(self._supervisor.processes)
