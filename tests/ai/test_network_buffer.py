##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## test_network_buffer
##

import sys
import os
import unittest
from collections import deque

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from NetworkBuffer import NetworkBuffer
from tcpClient import TcpClient


class FakeClient(TcpClient):
    """Faux TcpClient: recv_chunk() débite les chunks pré-programmés un par un,
    puis renvoie "" (socket vide, non-bloquant)."""

    def __init__(self, chunks):
        super().__init__(host="localhost", port=0)
        self._chunks = deque(chunks)

    def recv_chunk(self) -> str:
        if self._chunks:
            return self._chunks.popleft()
        return ""


def drain_all(buf: NetworkBuffer) -> None:
    """Vide le faux socket comme le ferait la boucle de jeu."""
    for _ in range(100):
        buf.poll()


class TestNetworkBuffer(unittest.TestCase):
    def test_single_line_response(self):
        buf = NetworkBuffer(FakeClient(["ok\n"]))
        buf.poll()
        self.assertEqual(buf.next_response(), "ok")
        self.assertIsNone(buf.next_response())

    def test_multiple_lines_one_chunk(self):
        # recv peut livrer plusieurs lignes d'un coup -> toutes traitées (while, pas if)
        buf = NetworkBuffer(FakeClient(["ok\nko\nok\n"]))
        buf.poll()
        self.assertEqual(buf.next_response(), "ok")
        self.assertEqual(buf.next_response(), "ko")
        self.assertEqual(buf.next_response(), "ok")
        self.assertIsNone(buf.next_response())

    def test_fragmented_line(self):
        # une ligne coupée sur plusieurs chunks doit être reconstruite
        buf = NetworkBuffer(FakeClient(["mess", "age 0", ", hi\n"]))
        drain_all(buf)
        self.assertEqual(buf.next_event(), "message 0, hi")
        self.assertIsNone(buf.next_event())

    def test_trailing_fragment_not_emitted(self):
        # un fragment sans \n final reste en attente, rien n'est émis
        client = FakeClient(["ok\npartial"])
        buf = NetworkBuffer(client)
        buf.poll()
        self.assertEqual(buf.next_response(), "ok")
        self.assertIsNone(buf.next_response())
        # le fragment se complète au chunk suivant
        client._chunks.append(" done\n")
        buf.poll()
        self.assertEqual(buf.next_response(), "partial done")

    def test_routing_events_vs_responses(self):
        stream = "ok\nmessage 2, salut\neject: 4\nko\ndead\n[player, food]\n"
        buf = NetworkBuffer(FakeClient([stream]))
        buf.poll()
        self.assertEqual(list(buf.response_queue), ["ok", "ko", "[player, food]"])
        self.assertEqual(
            list(buf.event_queue), ["message 2, salut", "eject: 4", "dead"]
        )

    def test_fifo_order_preserved(self):
        buf = NetworkBuffer(FakeClient(["ok\nko\n"]))
        buf.poll()
        # l'ordre d'arrivée = l'ordre de sortie (corrélation FIFO avec les commandes)
        self.assertEqual(buf.next_response(), "ok")
        self.assertEqual(buf.next_response(), "ko")

    def test_empty_socket_does_not_freeze(self):
        # socket vide -> poll() rend la main immédiatement, files vides
        buf = NetworkBuffer(FakeClient([]))
        buf.poll()
        self.assertIsNone(buf.next_response())
        self.assertIsNone(buf.next_event())

    def test_rapid_fragmented_stream(self):
        # DoD: flux rapide et fragmenté à l'extrême (1 octet par chunk)
        full = "ok\nmessage 1, x\nko\n"
        buf = NetworkBuffer(FakeClient(list(full)))
        drain_all(buf)
        self.assertEqual(list(buf.response_queue), ["ok", "ko"])
        self.assertEqual(list(buf.event_queue), ["message 1, x"])


if __name__ == "__main__":
    unittest.main()
