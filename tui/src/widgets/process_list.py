from textual.widgets import OptionList
from textual.widgets.option_list import Option

from supervisor.process import ManagedProcess


class ProcessList(OptionList):
    """Left-bottom pane: the running children, one per row. Rebuilt from the
    supervisor's process list on every refresh; row order matches it so the
    highlighted index maps straight back to a process."""

    def on_mount(self) -> None:
        self.border_title = "Processes"

    def show(self, processes: list[ManagedProcess]) -> None:
        keep = self.highlighted
        self.clear_options()
        self.add_options([Option(self._label(p)) for p in processes])
        if processes:
            self.highlighted = min(keep or 0, len(processes) - 1)

    @staticmethod
    def _label(process: ManagedProcess) -> str:
        state = "alive" if process.is_alive() else "dead"
        return f"{process.name:12} pid {process.pid}  {state}"
