import subprocess


class ProcessError(Exception):
    """Raised when a managed process is driven into an illegal state."""


class ManagedProcess:
    """One spawned child: start it once, check if alive, stop it cleanly.
    stdout+stderr merged into one pipe — a reader must drain it or a full
    buffer (~64 KiB) blocks the child."""

    def __init__(self, name: str, command: list[str]) -> None:
        if not command:
            raise ValueError("command must not be empty")
        self.name = name
        self.command = command
        self._proc: subprocess.Popen | None = None

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
        """Stop if running: SIGTERM, wait up to ``timeout``, then SIGKILL.
        Closes the output pipe. Idempotent."""
        if self._proc is None:
            return
        if self._proc.poll() is None:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
        if self._proc.stdout is not None:
            self._proc.stdout.close()
