from rich.text import Text
from textual.widgets import OptionList
from textual.widgets.option_list import Option

from supervisor.launcher import Launch
from supervisor.process import ManagedProcess


class ProcessList(OptionList):
    """Left-bottom pane: running children grouped by the game that launched
    them. Each game is a disabled header row (skipped by the cursor) followed
    by its processes; a row→process map turns the highlighted index back into
    a process (or None for a header)."""

    def __init__(self) -> None:
        super().__init__()
        self._rows: list[ManagedProcess | None] = []

    def on_mount(self) -> None:
        self.border_title = "Processes"

    def show(self, games: list[tuple[str, Launch]]) -> None:
        keep = self.highlighted
        self.clear_options()
        self._rows = []
        for name, launch in games:
            self._add(Text(f"{name}  (port {launch.port})", style="bold"), None, True)
            for process in self._members(launch):
                self._add(self._label(process), process, False)
        self.highlighted = self._keep_or_first_process(keep)

    def _keep_or_first_process(self, keep: int | None) -> int | None:
        """Hold the previous selection if it still points at a process,
        otherwise land on the first process row (never a disabled header)."""
        if keep is not None and 0 <= keep < len(self._rows) and self._rows[keep]:
            return keep
        return next((i for i, p in enumerate(self._rows) if p is not None), None)

    def process_at(self, index: int) -> ManagedProcess | None:
        if 0 <= index < len(self._rows):
            return self._rows[index]
        return None

    def _add(self, prompt: Text, process: ManagedProcess | None, disabled: bool) -> None:
        self.add_option(Option(prompt, disabled=disabled))
        self._rows.append(process)

    @staticmethod
    def _members(launch: Launch) -> list[ManagedProcess]:
        members = [launch.server, *launch.ais]
        if launch.gui is not None:
            members.append(launch.gui)
        return members

    @staticmethod
    def _label(process: ManagedProcess) -> Text:
        alive = process.is_alive()
        text = Text(f"  {process.name:12} ")
        text.append("alive" if alive else "dead", style="green" if alive else "red")
        return text
