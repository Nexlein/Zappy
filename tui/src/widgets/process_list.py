from rich.text import Text
from textual.widgets import OptionList
from textual.widgets.option_list import Option

from supervisor.launcher import Launch
from supervisor.manager import Game
from supervisor.process import ManagedProcess


class ProcessList(OptionList):
    """Live children grouped by game. Only games with an alive member show.
    Game headers are selectable rows (highlighting one selects the whole game
    for the status panel and attach/stop actions); member rows map to a single
    process. Two parallel maps decode the cursor: a row → process and row →
    game, the latter set for headers and members alike."""

    def __init__(self) -> None:
        super().__init__()
        self._rows: list[ManagedProcess | None] = []
        self._games: list[Game | None] = []
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
        self._games = []
        for game in games:
            if not self._members(game.launch):
                continue
            self._add(self._header(game), None, game)
            self._add_members(game)
        self.highlighted = self._keep_or_first(keep)
        self.scroll_to(y=scroll_y, animate=False)

    def _add_members(self, game: Game) -> None:
        launch = game.launch
        if launch.server.is_alive():
            self._add(self._member(launch.server, "server"), launch.server, game)
        for ai in launch.ais:
            if ai.is_alive():
                self._add(self._member(ai, "ai"), ai, game)
        for gui in launch.guis:
            if gui.is_alive():
                self._add(self._member(gui, "gui"), gui, game)

    @staticmethod
    def _header(game: Game) -> Text:
        text = Text(no_wrap=True)
        text.append("▸ ", style="bold yellow")
        text.append(game.name, style="bold cyan")
        text.append(f"  port {game.launch.port}", style="dim")
        return text

    @staticmethod
    def _member(process: ManagedProcess, role: str) -> Text:
        color = {"server": "green", "ai": "magenta", "gui": "blue"}[role]
        text = Text(no_wrap=True)
        text.append("    ")
        text.append("● " if process.is_alive() else "○ ", style=color)
        text.append(process.name, style=color)
        return text

    @staticmethod
    def _signature_of(games: list[Game]) -> tuple:
        rows: list[tuple] = []
        for game in games:
            members = ProcessList._members(game.launch)
            if not members:
                continue
            rows.append((game.name, game.launch.port))
            rows.extend((p.name, p.is_alive()) for p in members)
        return tuple(rows)

    def _keep_or_first(self, keep: int | None) -> int | None:
        """Hold the previous cursor if still in range, else land on the first
        row (a header — now selectable, so the first game is always reachable)."""
        if keep is not None and 0 <= keep < len(self._rows):
            return keep
        return 0 if self._rows else None

    def process_at(self, index: int) -> ManagedProcess | None:
        if 0 <= index < len(self._rows):
            return self._rows[index]
        return None

    def game_at(self, index: int) -> Game | None:
        if 0 <= index < len(self._games):
            return self._games[index]
        return None

    def _add(
        self, prompt: Text, process: ManagedProcess | None, game: Game | None
    ) -> None:
        self.add_option(Option(prompt))
        self._rows.append(process)
        self._games.append(game)

    @staticmethod
    def _members(launch: Launch) -> list[ManagedProcess]:
        members = [launch.server, *launch.ais, *launch.guis]
        return [p for p in members if p.is_alive()]
