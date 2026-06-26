##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## CLI parser tests
##

import sys
import os
import unittest
from unittest.mock import patch

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from argsParser import parseArgs, Config


class TestParseArgs(unittest.TestCase):
    def test_all_args(self):
        with patch(
            "sys.argv",
            [
                "zappy_ai",
                "-p",
                "4242",
                "-n",
                "team1",
                "-h",
                "192.168.1.1",
                "-s",
                "utility",
            ],
        ):
            config = parseArgs()
            self.assertEqual(
                config,
                Config(
                    port=4242,
                    teamName="team1",
                    host="192.168.1.1",
                    strategy="utility",
                    verbose=None,
                ),
            )

    @patch(
        "argsParser.get_client_config",
        return_value={"port": 8080, "teamName": "JsonTeam"},
    )
    def test_zero_args_uses_defaults(self, mock_defaults):
        with patch("sys.argv", ["zappy_ai"]):
            config = parseArgs()
            self.assertEqual(config.port, 8080)
            self.assertEqual(config.teamName, "JsonTeam")
            self.assertEqual(config.host, "localhost")

    def test_default_host_and_strategy(self):
        with patch("sys.argv", ["zappy_ai", "-p", "4242", "-n", "team1"]):
            config = parseArgs()
            self.assertEqual(config.host, "localhost")
            self.assertEqual(config.strategy, "fsm")
            self.assertIsNone(config.verbose)

    def test_verbose_flag(self):
        with patch("sys.argv", ["zappy_ai", "-p", "4242", "-n", "team1", "-v"]):
            config = parseArgs()
            self.assertEqual(config.verbose, "both")

        with patch("sys.argv", ["zappy_ai", "-p", "4242", "-n", "team1", "-v", "ai"]):
            config = parseArgs()
            self.assertEqual(config.verbose, "ai")

    @patch("argsParser.get_client_config", return_value={})
    def test_missing_port(self, mock_defaults):
        with patch("sys.argv", ["zappy_ai", "-n", "team1"]):
            with self.assertRaises(SystemExit):
                parseArgs()

    @patch("argsParser.get_client_config", return_value={})
    def test_missing_name(self, mock_defaults):
        with patch("sys.argv", ["zappy_ai", "-p", "4242"]):
            with self.assertRaises(SystemExit):
                parseArgs()

    def test_invalid_port(self):
        with patch("sys.argv", ["zappy_ai", "-p", "notaport", "-n", "team1"]):
            with self.assertRaises(SystemExit):
                parseArgs()

    def test_help(self):
        with patch("sys.argv", ["zappy_ai", "--help"]):
            with self.assertRaises(SystemExit) as ctx:
                parseArgs()
            self.assertEqual(ctx.exception.code, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
