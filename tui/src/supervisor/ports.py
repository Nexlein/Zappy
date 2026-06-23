import socket

_MAX_PORT = 65535


class PortError(Exception):
    """Raised when no free port can be allocated in the configured range."""


class PortAllocator:
    """Hands out TCP ports from a fixed range, tracking what it gave out.

    Two layers of collision avoidance:
    - a ``_reserved`` set, so the allocator never returns the same port twice
      even before the caller has bound it;
    - a live bind probe, so it never returns a port already taken by another
      process on the machine.
    """

    def __init__(self, start: int = 8000, end: int = 9000) -> None:
        # end is exclusive, so a valid range tops out at _MAX_PORT + 1.
        if start <= 0 or end <= start or end > _MAX_PORT + 1:
            raise ValueError(f"invalid port range [{start}, {end})")
        self._start = start
        self._end = end
        self._reserved: set[int] = set()

    def allocate(self) -> int:
        """Reserve and return the lowest free port, or raise PortError."""
        for port in range(self._start, self._end):
            if port in self._reserved:
                continue
            if self._is_free(port):
                self._reserved.add(port)
                return port
        raise PortError(f"no free port in range [{self._start}, {self._end})")

    def release(self, port: int) -> None:
        """Give a port back to the pool. Idempotent — releasing an unknown
        port is a no-op, not an error."""
        self._reserved.discard(port)

    @staticmethod
    def _is_free(port: int) -> bool:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
            try:
                probe.bind(("", port))
                return True
            except OSError:
                return False
