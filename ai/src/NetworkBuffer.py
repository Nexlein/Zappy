##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## NetworkBuffer
##

from collections import deque
from dataclasses import dataclass, field
from tcpClient import TcpClient

EVENT_PREFIXES = (
    "message",
    "eject",
    "dead",
    "Elevation underway",
    "Current level:",
)


@dataclass
class NetworkBuffer:
    _client: TcpClient
    _buffer: str = ""
    response_queue: deque[str] = field(default_factory=deque)
    event_queue: deque[str] = field(default_factory=deque)

    def poll(self) -> None:
        """Read whatever data is available on the socket (non-blocking)."""
        chunk = self._client.recv_chunk()
        if chunk:
            self._buffer += chunk
            self._drain()

    def send_command(self, command: str) -> None:
        """Send a command to the server."""
        self._client.send(command)

    def next_event(self) -> str | None:
        """Pop and return the oldest unsolicited event, or None."""
        if self.event_queue:
            return self.event_queue.popleft()
        return None

    def next_response(self) -> str | None:
        """Pop and return the oldest command response, or None."""
        if self.response_queue:
            return self.response_queue.popleft()
        return None

    def _drain(self) -> None:
        """Process all complete lines currently in the internal buffer."""
        while "\n" in self._buffer:
            line, self._buffer = self._buffer.split("\n", 1)
            self._route(line.strip())

    def _route(self, line: str) -> None:
        """Route a complete line to the appropriate queue."""
        if not line:
            return
        if line.startswith(EVENT_PREFIXES):
            self.event_queue.append(line)
        else:
            self.response_queue.append(line)
