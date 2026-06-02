##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## tcpClient
##

from dataclasses import dataclass
import select
import socket


@dataclass
class TcpClient:
    host: str
    port: int
    _socket: socket.socket | None = None
    _buffer: str = ""

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
            chunk = self._socket.recv(1024)
            if not chunk:
                raise ConnectionError("server closed connection")
            self._buffer += chunk.decode()
        line, self._buffer = self._buffer.split("\n", 1)
        return line

    def recv_chunk(self) -> str:
        if self._socket is None:
            raise RuntimeError("not connected")
        readable, _, _ = select.select([self._socket], [], [], 0)
        if not readable:
            return ""
        chunk = self._socket.recv(1024)
        if not chunk:
            raise ConnectionError("server closed connection")
        return chunk.decode()

    def handshake(self, team_name: str):
        message = self.receive()

        if message != "WELCOME":
            raise ConnectionError(f"unexpected handshake message: {message}")
        self.send(team_name)

        available_slots = int(self.receive())

        dimensions = self.receive()
        x, y = dimensions.split()
        map_width, map_height = int(x), int(y)
        if self._socket is not None:
            self._socket.setblocking(False)
        return available_slots, (map_width, map_height)
