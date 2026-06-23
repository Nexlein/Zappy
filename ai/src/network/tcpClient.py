##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## tcpClient
##

from dataclasses import dataclass
import select
import socket
from utils.config_loader import get_network_config


@dataclass
class TcpClient:
    host: str
    port: int
    _socket: socket.socket | None = None
    _buffer: str = ""
    _recv_buffer_size: int = 4096

    def __post_init__(self):
        self._recv_buffer_size = get_network_config().get("TCP_RECV_BUFFER_SIZE", 4096)

    def connect(self):
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.connect((self.host, self.port))

    def send(self, data: str):
        if self._socket is None:
            raise RuntimeError("not connected")
        data = data if data.endswith("\n") else data + "\n"
        self._socket.sendall(data.encode())

    def receive(self) -> str:
        if self._socket is None:
            raise RuntimeError("not connected")
        while "\n" not in self._buffer:
            chunk = self._socket.recv(self._recv_buffer_size)
            if not chunk:
                raise ConnectionError("server closed connection")
            self._buffer += chunk.decode()
        line, self._buffer = self._buffer.split("\n", 1)
        return line

    def recv_chunk(self) -> str:
        if self._socket is None:
            raise RuntimeError("not connected")

        if self._buffer:
            res = self._buffer
            self._buffer = ""
            return res

        readable, _, _ = select.select([self._socket], [], [], 0)
        if not readable:
            return ""
        chunk = self._socket.recv(self._recv_buffer_size)
        if not chunk:
            raise ConnectionError("server closed connection")
        return chunk.decode()

    def handshake(self, team_name: str):
        message = self.receive()

        if message != "WELCOME":
            raise ConnectionError(f"unexpected handshake message: {message}")
        self.send(team_name)

        slots_str = self.receive()
        if slots_str == "ko":
            raise ConnectionRefusedError("Team is full")
        available_slots = int(slots_str)

        dimensions = self.receive()
        x, y = dimensions.split()
        map_width, map_height = int(x), int(y)
        if self._socket is not None:
            self._socket.setblocking(False)
        return available_slots, (map_width, map_height)
