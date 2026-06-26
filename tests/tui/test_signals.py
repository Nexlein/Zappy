##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test signal guard
##

import os
import signal
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.signals import raise_on_signals


class TestRaiseOnSignals(unittest.TestCase):
    def test_signal_becomes_systemexit(self):
        with self.assertRaises(SystemExit) as ctx:
            with raise_on_signals(signal.SIGUSR1):
                os.kill(os.getpid(), signal.SIGUSR1)
        self.assertEqual(ctx.exception.code, 128 + signal.SIGUSR1)

    def test_previous_handler_restored(self):
        before = signal.getsignal(signal.SIGUSR1)
        with raise_on_signals(signal.SIGUSR1):
            self.assertNotEqual(signal.getsignal(signal.SIGUSR1), before)
        self.assertEqual(signal.getsignal(signal.SIGUSR1), before)

    def test_restored_even_on_exception(self):
        before = signal.getsignal(signal.SIGUSR1)
        with self.assertRaises(SystemExit):
            with raise_on_signals(signal.SIGUSR1):
                os.kill(os.getpid(), signal.SIGUSR1)
        self.assertEqual(signal.getsignal(signal.SIGUSR1), before)


if __name__ == "__main__":
    unittest.main()
