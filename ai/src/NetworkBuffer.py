##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## NetworkBuffer
##

from collections import deque
from dataclasses import dataclass, field
from tcpClient import TcpClient

EVENT_PREFIXES = ("message", "eject", "dead")


@dataclass
class NetworkBuffer:
    _client: TcpClient
    _buffer: str = ""
    response_queue: deque[str] = field(default_factory=deque)
    event_queue: deque[str] = field(default_factory=deque)

    def poll(self) -> None:
        chunk = self._client.recv_chunk()
        if chunk:
            self._buffer += chunk
            self._drain()

    def next_event(self) -> str | None:
        if self.event_queue:
            return self.event_queue.popleft()
        return None

    def next_response(self) -> str | None:
        if self.response_queue:
            return self.response_queue.popleft()
        return None

    def _drain(self) -> None:
        while "\n" in self._buffer:
            line, self._buffer = self._buffer.split("\n", 1)
            self._route(line)

    def _route(self, line: str) -> None:
        if line.startswith(EVENT_PREFIXES):
            self.event_queue.append(line)
        else:
            self.response_queue.append(line)
