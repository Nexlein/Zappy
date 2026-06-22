##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test Integration
##

import unittest
import socket
import threading
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from network.tcpClient import TcpClient


class DummyZappyServer:
    def __init__(self, host="127.0.0.1", port=0):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.bind((host, port))
        self.server_socket.listen(1)
        self.port = self.server_socket.getsockname()[1]
        self.thread = threading.Thread(target=self._run)
        self.thread.daemon = True
        self.running = False

    def start(self):
        self.running = True
        self.thread.start()

    def _run(self):
        try:
            conn, addr = self.server_socket.accept()
            with conn:
                # Step 1: Send WELCOME
                conn.sendall(b"WELCOME\n")

                # Step 2: Receive Team Name
                buffer = ""
                while "\n" not in buffer:
                    chunk = conn.recv(1024)
                    if not chunk:
                        break
                    buffer += chunk.decode()

                # Step 3: Send Remaining Slots
                conn.sendall(b"0\n")

                # Step 4: Send Dimensions
                conn.sendall(b"10 10\n")
        except Exception:
            pass
        finally:
            self.server_socket.close()

    def stop(self):
        self.running = False


class TestIntegration(unittest.TestCase):
    def test_tcp_handshake_integration(self):
        """
        Tests the full TCP handshake against a local dummy server thread.
        This verifies that sending and receiving happens in the correct order,
        and that a 0-slot response is successfully handled as a valid connection.
        """
        server = DummyZappyServer()
        server.start()

        try:
            client = TcpClient(host="127.0.0.1", port=server.port)
            client.connect()

            slots, (w, h) = client.handshake("test_team")

            self.assertEqual(slots, 0)
            self.assertEqual(w, 10)
            self.assertEqual(h, 10)

        finally:
            server.stop()


if __name__ == "__main__":
    unittest.main()
