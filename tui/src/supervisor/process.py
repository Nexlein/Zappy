import subprocess
import threading
from collections import deque

_LOG_LINES = 2000  # ring-buffer size per process


class ProcessError(Exception):
    """Raised when a managed process is driven into an illegal state."""


class ManagedProcess:
    """One spawned child: start it once, check if alive, stop it cleanly.
    A reader thread drains the child's merged stdout+stderr into a bounded ring
    buffer (also stops the ~64 KiB pipe from blocking the child)."""

    def __init__(self, name: str, command: list[str]) -> None:
        if not command:
            raise ValueError("command must not be empty")
        self.name = name
        self.command = command
        self._proc: subprocess.Popen | None = None
        self._log: deque[str] = deque(maxlen=_LOG_LINES)
        self._log_seq = 0  # total lines ever read, including dropped ones
        self._log_lock = threading.Lock()
        self._reader: threading.Thread | None = None

    def start(self) -> None:
        """Spawn the child. Raises ProcessError if already started."""
        if self._proc is not None:
            raise ProcessError(f"{self.name}: already started")
        self._proc = subprocess.Popen(
            self.command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        self._reader = threading.Thread(target=self._drain, daemon=True)
        self._reader.start()

    def _drain(self) -> None:
        """Read the pipe line by line until EOF (child died / pipe closed)."""
        assert self._proc is not None and self._proc.stdout is not None
        try:
            for line in self._proc.stdout:
                with self._log_lock:
                    self._log.append(line.rstrip("\n"))
                    self._log_seq += 1
        except (ValueError, OSError):
            pass  # pipe closed from under us

    def log_snapshot(self) -> tuple[int, list[str]]:
        """The total line count ever seen and the currently buffered lines.
        The count lets a reader append only what is new across polls."""
        with self._log_lock:
            return self._log_seq, list(self._log)

    def is_alive(self) -> bool:
        """True if the child has been started and has not yet exited."""
        return self._proc is not None and self._proc.poll() is None

    @property
    def pid(self) -> int | None:
        """OS pid once started, else None."""
        return self._proc.pid if self._proc is not None else None

    @property
    def returncode(self) -> int | None:
        """Exit code if the child has exited, else None (running or unstarted)."""
        if self._proc is None:
            return None
        return self._proc.poll()

    def stop(self, timeout: float = 5.0) -> None:
        """Stop if running: SIGTERM, wait up to ``timeout``, then SIGKILL. Lets
        the reader drain the last output, then closes the pipe. Idempotent."""
        if self._proc is None:
            return
        if self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
        if self._reader is not None:
            self._reader.join(timeout=1.0)  # child dead -> pipe EOF -> reader ends
        if self._proc.stdout is not None:
            self._proc.stdout.close()
