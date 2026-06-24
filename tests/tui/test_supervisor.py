##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test supervisor
##

import os
import sys
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.ports import PortAllocator
from supervisor.supervisor import Supervisor


def _wait_dead(process, timeout=2.0):
    deadline = time.monotonic() + timeout
    while process.is_alive() and time.monotonic() < deadline:
        time.sleep(0.01)


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

    def test_reap_returns_dead_keeps_entry(self):
        with Supervisor() as sup:
            p = sup.spawn("quick", ["true"])
            _wait_dead(p)
            self.assertEqual(sup.reap(), [p])
            # entry kept so the UI can still show the dead process
            self.assertEqual(sup.processes, [p])

    def test_reap_ignores_alive(self):
        with Supervisor() as sup:
            sup.spawn("sleep", ["sleep", "30"])
            self.assertEqual(sup.reap(), [])

    def test_reap_is_idempotent(self):
        with Supervisor() as sup:
            p = sup.spawn("quick", ["true"])
            _wait_dead(p)
            self.assertEqual(sup.reap(), [p])
            self.assertEqual(sup.reap(), [])  # already reaped

    def test_reap_frees_port_for_reuse(self):
        ports = PortAllocator(8770, 8771)  # single-port range
        with Supervisor(ports) as sup:
            port = sup.ports.allocate()
            p = sup.spawn("server", ["true"], port=port)
            _wait_dead(p)
            sup.reap()
            self.assertEqual(ports.allocate(), port)


if __name__ == "__main__":
    unittest.main()
