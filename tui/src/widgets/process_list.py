from rich.text import Text
from textual.widgets import OptionList
from textual.widgets.option_list import Option

from supervisor.launcher import Launch
from supervisor.manager import Game
from supervisor.process import ManagedProcess


class ProcessList(OptionList):
    """Live children grouped by game. Only alive processes show; dead ones
    vanish. Headers are disabled rows; a row→process map decodes the cursor."""

    def __init__(self) -> None:
        super().__init__()
        self._rows: list[ManagedProcess | None] = []
        self._signature: tuple | None = None

    def on_mount(self) -> None:
        self.border_title = "Processes"

    def show(self, games: list[Game]) -> None:
        # Rebuilding resets scroll, so only do it when the rendered set changed.
        signature = self._signature_of(games)
        if signature == self._signature:
            return
        self._signature = signature

        keep = self.highlighted
        scroll_y = self.scroll_offset.y
        self.clear_options()
        self._rows = []
        for game in games:
            alive = self._members(game.launch)
            if not alive:
                continue
            header = Text(f"{game.name}  (port {game.launch.port})", style="bold")
            self._add(header, None, True)
            for process in alive:
                self._add(Text(f"  {process.name}"), process, False)
        self.highlighted = self._keep_or_first_process(keep)
        self.scroll_to(y=scroll_y, animate=False)

    @staticmethod
    def _signature_of(games: list[Game]) -> tuple:
        return tuple(
            (game.name, game.launch.port, p.name)
            for game in games
            for p in ProcessList._members(game.launch)
        )

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

    def _add(
        self, prompt: Text, process: ManagedProcess | None, disabled: bool
    ) -> None:
        self.add_option(Option(prompt, disabled=disabled))
        self._rows.append(process)

    @staticmethod
    def _members(launch: Launch) -> list[ManagedProcess]:
        members = [launch.server, *launch.ais, *launch.guis]
        return [p for p in members if p.is_alive()]
