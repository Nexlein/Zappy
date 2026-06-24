##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test command builders
##

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.commands import ai_command, gui_command, server_command
from supervisor.profiles import Profile, Team


def _profile():
    return Profile(
        name="duel",
        width=20,
        height=10,
        clients=6,
        freq=100,
        auto_gui=False,
        teams=(Team("red", 2), Team("blue", 1)),
    )


class TestCommands(unittest.TestCase):
    def test_server_command(self):
        self.assertEqual(
            server_command("./zappy_server", 8000, _profile()),
            ["./zappy_server", "-p", "8000", "-x", "20", "-y", "10",
             "-n", "red", "blue", "-c", "6", "-f", "100"],
        )

    def test_ai_command_local(self):
        self.assertEqual(
            ai_command("./zappy_ai", 8000, "red"),
            ["./zappy_ai", "-p", "8000", "-n", "red"],
        )

    def test_ai_command_with_host(self):
        self.assertEqual(
            ai_command("./zappy_ai", 8000, "red", host="10.0.0.1"),
            ["./zappy_ai", "-p", "8000", "-n", "red", "-h", "10.0.0.1"],
        )

    def test_gui_command_local(self):
        self.assertEqual(
            gui_command("./zappy_gui", 8000),
            ["./zappy_gui", "-p", "8000"],
        )

    def test_gui_command_with_host(self):
        self.assertEqual(
            gui_command("./zappy_gui", 8000, host="10.0.0.1"),
            ["./zappy_gui", "-p", "8000", "-h", "10.0.0.1"],
        )


if __name__ == "__main__":
    unittest.main()
