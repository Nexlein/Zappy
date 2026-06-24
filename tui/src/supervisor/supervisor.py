from __future__ import annotations

from .ports import PortAllocator
from .process import ManagedProcess


class Supervisor:
    """Tracks every spawned child and the port it owns, so one shutdown leaves
    no zombies or orphaned ports. Use as a context manager for crash-safe
    cleanup. Stays ignorant of CLI flags: the caller builds the command and
    passes back any allocated ``port`` only so it gets released on stop."""

    def __init__(self, ports: PortAllocator | None = None) -> None:
        self.ports = ports or PortAllocator()
        self._entries: list[_Entry] = []

    def spawn(
        self, name: str, command: list[str], *, port: int | None = None
    ) -> ManagedProcess:
        """Create, start and track a child."""
        process = ManagedProcess(name, command)
        process.start()
        self._entries.append(_Entry(process, port))
        return process

    @property
    def processes(self) -> list[ManagedProcess]:
        """Every tracked child, in spawn order."""
        return [entry.process for entry in self._entries]

    def reap(self) -> list[ManagedProcess]:
        """Free the resources of children that have exited on their own:
        close their pipe and release their port. Entries are kept (marked
        dead) so the UI can still show them. Returns the newly reaped ones."""
        reaped = []
        for entry in self._entries:
            if entry.released or entry.process.is_alive():
                continue
            entry.process.stop()  # already dead: just closes the pipe
            self._release(entry)
            reaped.append(entry.process)
        return reaped

    def shutdown(self) -> None:
        """Stop every child and release its port. Idempotent."""
        for entry in self._entries:
            entry.process.stop()
            self._release(entry)
        self._entries.clear()

    def _release(self, entry: _Entry) -> None:
        """Release an entry's port once. Idempotent across reap and shutdown."""
        if entry.released:
            return
        if entry.port is not None:
            self.ports.release(entry.port)
        entry.released = True

    def __enter__(self) -> Supervisor:
        return self

    def __exit__(self, *exc: object) -> None:
        self.shutdown()


class _Entry:
    __slots__ = ("process", "port", "released")

    def __init__(self, process: ManagedProcess, port: int | None) -> None:
        self.process = process
        self.port = port
        self.released = False
