from textual.app import App, ComposeResult
from textual.containers import Horizontal, Vertical
from textual.widgets import Footer, Header

from supervisor.profiles import Profile
from widgets.detail_panel import DetailPanel
from widgets.process_list import ProcessList
from widgets.profile_list import ProfileList


class ZappyTUI(App):
    TITLE = "Zappy orchestrator"
    CSS_PATH = "app.tcss"
    BINDINGS = [("q", "quit", "Quit")]

    def __init__(self, profiles: dict[str, Profile]) -> None:
        super().__init__()
        self._profiles = profiles

    def compose(self) -> ComposeResult:
        yield Header()
        with Horizontal():
            with Vertical(id="sidebar"):
                yield ProfileList(self._profiles)
                yield ProcessList()
            yield DetailPanel()
        yield Footer()
