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

    def shutdown(self) -> None:
        """Stop every child and release its port. Idempotent."""
        for entry in self._entries:
            entry.process.stop()
            if entry.port is not None:
                self.ports.release(entry.port)
        self._entries.clear()

    def __enter__(self) -> Supervisor:
        return self

    def __exit__(self, *exc: object) -> None:
        self.shutdown()


class _Entry:
    __slots__ = ("process", "port")

    def __init__(self, process: ManagedProcess, port: int | None) -> None:
        self.process = process
        self.port = port
