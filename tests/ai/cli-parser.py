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
        with patch("sys.argv", ["zappy_ai", "-p", "4242", "-n", "team1", "-h", "192.168.1.1"]):
            config = parseArgs()
            self.assertEqual(config, Config(port=4242, teamName="team1", host="192.168.1.1"))

    def test_default_host(self):
        with patch("sys.argv", ["zappy_ai", "-p", "4242", "-n", "team1"]):
            config = parseArgs()
            self.assertEqual(config.host, "localhost")

    def test_missing_port(self):
        with patch("sys.argv", ["zappy_ai", "-n", "team1"]):
            with self.assertRaises(SystemExit):
                parseArgs()

    def test_missing_name(self):
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
