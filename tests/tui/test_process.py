##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test managed process
##

import os
import sys
import time
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.process import ManagedProcess, ProcessError


class TestManagedProcess(unittest.TestCase):
    def test_empty_command_rejected(self):
        with self.assertRaises(ValueError):
            ManagedProcess("x", [])

    def test_unstarted_state(self):
        p = ManagedProcess("x", ["true"])
        self.assertFalse(p.is_alive())
        self.assertIsNone(p.pid)
        self.assertIsNone(p.returncode)
        self.assertIsNone(p.uptime)

    def test_uptime_runs_then_freezes(self):
        p = ManagedProcess("sleep", ["sleep", "30"])
        p.start()
        try:
            first = p.uptime
            self.assertIsNotNone(first)
            time.sleep(0.05)
            self.assertGreater(p.uptime, first)
        finally:
            p.stop()
        # dead child: uptime stops advancing
        frozen = p.uptime
        time.sleep(0.05)
        self.assertEqual(p.uptime, frozen)

    def test_start_sets_pid_and_alive(self):
        p = ManagedProcess("sleep", ["sleep", "5"])
        p.start()
        try:
            self.assertIsInstance(p.pid, int)
            self.assertTrue(p.is_alive())
            self.assertIsNone(p.returncode)
        finally:
            p.stop()

    def test_double_start_raises(self):
        p = ManagedProcess("sleep", ["sleep", "5"])
        p.start()
        try:
            with self.assertRaises(ProcessError):
                p.start()
        finally:
            p.stop()

    def test_natural_exit_reaped(self):
        p = ManagedProcess("true", ["true"])
        p.start()
        # poll until the child exits; is_alive must reap it
        for _ in range(50):
            if not p.is_alive():
                break
            time.sleep(0.02)
        self.assertFalse(p.is_alive())
        self.assertEqual(p.returncode, 0)
        p.stop()  # close the output pipe of the already-dead child

    def test_stop_terminates_running_child(self):
        p = ManagedProcess("sleep", ["sleep", "30"])
        p.start()
        p.stop()
        self.assertFalse(p.is_alive())
        self.assertIsNotNone(p.returncode)

    def test_stop_is_idempotent(self):
        p = ManagedProcess("sleep", ["sleep", "30"])
        p.start()
        p.stop()
        p.stop()  # second call must not raise

    def test_stop_unstarted_is_noop(self):
        ManagedProcess("x", ["true"]).stop()

    def test_stop_kills_after_timeout(self):
        # traps SIGTERM and ignores it, so graceful wait expires -> SIGKILL
        p = ManagedProcess("stubborn", ["sh", "-c", "trap '' TERM; sleep 30"])
        p.start()
        p.stop(timeout=0.5)
        self.assertFalse(p.is_alive())

    def test_captures_output(self):
        p = ManagedProcess("echoer", ["sh", "-c", "echo one; echo two"])
        p.start()
        for _ in range(50):  # let the reader thread drain
            if p.log_snapshot()[0] >= 2:
                break
            time.sleep(0.02)
        seq, lines = p.log_snapshot()
        p.stop()
        self.assertEqual(seq, 2)
        self.assertEqual(lines, ["one", "two"])

    def test_log_snapshot_empty_before_output(self):
        p = ManagedProcess("x", ["sleep", "5"])
        p.start()
        try:
            self.assertEqual(p.log_snapshot(), (0, []))
        finally:
            p.stop()


if __name__ == "__main__":
    unittest.main()
