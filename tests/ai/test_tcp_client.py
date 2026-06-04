##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## TCP client tests
##

import sys
import os
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from tcpClient import TcpClient


class FakeSocket:
    """recv() renvoie les chunks programmés un par un, puis b'' (EOF)."""

    def __init__(self, chunks):
        self.chunks = list(chunks)
        self.sent = b""

    def recv(self, _bufsize):
        return self.chunks.pop(0) if self.chunks else b""

    def sendall(self, data):
        self.sent += data

    def connect(self, _addr):
        pass

    def setblocking(self, _blocking):
        pass


def make_client(chunks=None):
    client = TcpClient("localhost", 4242)
    client._socket = FakeSocket(chunks or [])
    return client


class TestSend(unittest.TestCase):
    def test_appends_newline(self):
        client = make_client()
        client.send("team1")
        self.assertEqual(client._socket.sent, b"team1\n")

    def test_does_not_double_newline(self):
        client = make_client()
        client.send("team1\n")
        self.assertEqual(client._socket.sent, b"team1\n")


class TestReceive(unittest.TestCase):
    def test_single_line(self):
        client = make_client([b"WELCOME\n"])
        self.assertEqual(client.receive(), "WELCOME")

    def test_line_split_across_chunks(self):
        client = make_client([b"WEL", b"CO", b"ME\n"])
        self.assertEqual(client.receive(), "WELCOME")

    def test_multiple_lines_one_recv(self):
        client = make_client([b"WELCOME\n42\n"])
        self.assertEqual(client.receive(), "WELCOME")
        self.assertEqual(client.receive(), "42")  # depuis le buffer, sans nouveau recv

    def test_raises_on_closed_connection(self):
        client = make_client([])  # recv() renverra b"" direct
        with self.assertRaises(ConnectionError):
            client.receive()

    def test_raises_on_partial_then_close(self):
        client = make_client([b"WEL"])  # données incomplètes puis EOF
        with self.assertRaises(ConnectionError):
            client.receive()


class TestHandshake(unittest.TestCase):
    def test_full_handshake(self):
        client = make_client([b"WELCOME\n", b"3\n", b"10 20\n"])
        slots, dims = client.handshake("team1")
        self.assertEqual(slots, 3)
        self.assertEqual(dims, (10, 20))

    def test_sends_team_name(self):
        client = make_client([b"WELCOME\n", b"3\n", b"10 20\n"])
        client.handshake("team1")
        self.assertEqual(client._socket.sent, b"team1\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
