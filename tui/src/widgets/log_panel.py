from textual.widgets import RichLog

from supervisor.process import ManagedProcess


class LogPanel(RichLog):
    """Right-bottom pane: tails the output of one selected process. ``follow``
    rebinds it; ``poll`` (called on the app timer) appends only new lines."""

    def __init__(self) -> None:
        super().__init__(max_lines=2000, wrap=True, auto_scroll=True)
        self._process: ManagedProcess | None = None
        self._seq = 0

    def on_mount(self) -> None:
        self.border_title = "Logs"

    def follow(self, process: ManagedProcess) -> None:
        self._process = process
        self.border_title = f"Logs — {process.name}"
        self.clear()
        self._seq, lines = process.log_snapshot()
        if not lines:
            self.write("(no output on stdout yet)")
            return
        for line in lines:
            self.write(line)

    def clear_follow(self) -> None:
        self._process = None
        self._seq = 0
        self.border_title = "Logs"
        self.clear()

    def poll(self) -> None:
        if self._process is None:
            return
        seq, lines = self._process.log_snapshot()
        new = seq - self._seq
        if new > 0:
            for line in lines[-new:]:  # clamp handles ring-buffer drops
                self.write(line)
            self._seq = seq
