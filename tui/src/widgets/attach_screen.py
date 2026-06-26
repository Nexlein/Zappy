from textual.app import ComposeResult
from textual.containers import Horizontal, Vertical
from textual.screen import ModalScreen
from textual.widgets import Button, Label, RadioButton, RadioSet

# zappy_ai -s choices; "default" maps to None (let the AI pick).
STRATEGIES = ["default", "fsm", "utility", "uai", "queen"]


class AttachAiScreen(ModalScreen[tuple[str, str | None] | None]):
    """Pick the team and algorithm for a new AI. Dismisses (team, strategy)
    where strategy is None for the default, or None when cancelled."""

    BINDINGS = [("escape", "cancel", "Cancel")]

    def __init__(
        self, teams: list[str], team: str, strategy: str | None = None
    ) -> None:
        super().__init__()
        self._teams = teams
        self._team = team
        self._strategy = strategy or "default"

    def compose(self) -> ComposeResult:
        with Vertical(id="attach-box"):
            yield Label("Attach AI")
            yield Label("Team", classes="attach-heading")
            yield _radio_set(self._teams, self._team, id="attach-team")
            yield Label("Algorithm", classes="attach-heading")
            yield _radio_set(STRATEGIES, self._strategy, id="attach-strategy")
            with Horizontal(id="attach-buttons"):
                yield Button("Attach", variant="primary", id="attach")
                yield Button("Cancel", id="cancel")

    def on_button_pressed(self, event: Button.Pressed) -> None:
        if event.button.id == "attach":
            self._confirm()
        else:
            self.dismiss(None)

    def action_cancel(self) -> None:
        self.dismiss(None)

    def _confirm(self) -> None:
        team = self._selected("#attach-team")
        strategy = self._selected("#attach-strategy")
        self.dismiss((team, None if strategy == "default" else strategy))

    def _selected(self, selector: str) -> str:
        button = self.query_one(selector, RadioSet).pressed_button
        return str(button.label)


def _radio_set(labels: list[str], selected: str, *, id: str) -> RadioSet:
    buttons = [RadioButton(label, value=label == selected) for label in labels]
    return RadioSet(*buttons, id=id)
