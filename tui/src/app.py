from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, Static

class ZappyTUI(App):
    TITLE = "Zappy orchestrator"

    def compose(self) -> ComposeResult:
        yield Header()
        yield Static("zappy tui scaffold")
        yield Footer()

