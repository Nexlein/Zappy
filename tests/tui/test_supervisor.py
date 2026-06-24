##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test supervisor
##

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.ports import PortAllocator
from supervisor.supervisor import Supervisor


class TestSupervisor(unittest.TestCase):
    def test_spawn_tracks_and_starts(self):
        with Supervisor() as sup:
            p = sup.spawn("sleep", ["sleep", "30"])
            self.assertTrue(p.is_alive())
            self.assertEqual(sup.processes, [p])

    def test_processes_in_spawn_order(self):
        with Supervisor() as sup:
            a = sup.spawn("a", ["sleep", "30"])
            b = sup.spawn("b", ["sleep", "30"])
            self.assertEqual(sup.processes, [a, b])

    def test_shutdown_kills_children(self):
        with Supervisor() as sup:
            p = sup.spawn("sleep", ["sleep", "30"])
        self.assertFalse(p.is_alive())
        self.assertEqual(sup.processes, [])

    def test_context_exit_kills_on_exception(self):
        sup = Supervisor()
        with self.assertRaises(RuntimeError):
            with sup:
                p = sup.spawn("sleep", ["sleep", "30"])
                raise RuntimeError("boom")
        self.assertFalse(p.is_alive())

    def test_shutdown_releases_port(self):
        # single-port range: shutdown must free it for reallocation
        ports = PortAllocator(8765, 8766)
        with Supervisor(ports) as sup:
            port = sup.ports.allocate()
            sup.spawn("server", ["sleep", "30"], port=port)
        self.assertEqual(ports.allocate(), port)

    def test_shutdown_idempotent(self):
        sup = Supervisor()
        sup.spawn("sleep", ["sleep", "30"])
        sup.shutdown()
        sup.shutdown()  # second call must not raise


if __name__ == "__main__":
    unittest.main()
