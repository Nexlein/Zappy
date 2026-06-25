from textual.widgets import RichLog

from supervisor.process import ManagedProcess

# Cap lines written to the widget per poll. A flooding child can produce
# thousands of lines a second; writing them all synchronously would block the
# event loop (UI freezes, Ctrl-C dies). We only ever show the latest few.
_MAX_WRITE_PER_POLL = 200


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
        for line in lines[-_MAX_WRITE_PER_POLL:]:
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
        if new <= 0:
            return
        self._seq = seq
        tail = lines[-new:] if new <= len(lines) else lines  # ring-buffer drops
        if len(tail) > _MAX_WRITE_PER_POLL:
            skipped = len(tail) - _MAX_WRITE_PER_POLL
            tail = tail[-_MAX_WRITE_PER_POLL:]
            self.write(f"… {skipped} line(s) skipped (flooding)")
        for line in tail:
            self.write(line)
