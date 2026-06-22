##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test profile loader
##

import os
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.profiles import ProfileError, load_profiles


class TestProfiles(unittest.TestCase):
    def setUp(self):
        self._dir = tempfile.TemporaryDirectory()
        self.dir = Path(self._dir.name)

    def tearDown(self):
        self._dir.cleanup()

    def write(self, content: str) -> Path:
        path = self.dir / "profiles.toml"
        path.write_text(textwrap.dedent(content))
        return path

    def test_loads_valid_profile(self):
        path = self.write(
            """
            [profiles.duel]
            width = 20
            height = 20
            clients = 6
            freq = 100
            auto_gui = true

            [[profiles.duel.teams]]
            name = "red"
            ai = 2

            [[profiles.duel.teams]]
            name = "blue"
            ai = 3
            """
        )
        duel = load_profiles(path)["duel"]
        self.assertEqual(duel.width, 20)
        self.assertIs(duel.auto_gui, True)
        self.assertEqual([t.name for t in duel.teams], ["red", "blue"])
        self.assertEqual(duel.teams[1].ai, 3)

    def test_auto_gui_defaults_false(self):
        path = self.write(
            """
            [profiles.solo]
            width = 10
            height = 10
            clients = 4
            freq = 50
            [[profiles.solo.teams]]
            name = "a"
            ai = 1
            """
        )
        self.assertIs(load_profiles(path)["solo"].auto_gui, False)

    def test_missing_file(self):
        with self.assertRaisesRegex(ProfileError, "not found"):
            load_profiles(self.dir / "nope.toml")

    def test_bool_rejected_as_int(self):
        path = self.write(
            """
            [profiles.bad]
            width = true
            height = 10
            clients = 4
            freq = 50
            [[profiles.bad.teams]]
            name = "a"
            ai = 1
            """
        )
        with self.assertRaisesRegex(ProfileError, "'width' must be an integer"):
            load_profiles(path)

    def test_unknown_key(self):
        path = self.write(
            """
            [profiles.bad]
            widht = 10
            height = 10
            clients = 4
            freq = 50
            [[profiles.bad.teams]]
            name = "a"
            ai = 1
            """
        )
        with self.assertRaisesRegex(ProfileError, "unknown key"):
            load_profiles(path)

    def test_ai_exceeds_clients(self):
        path = self.write(
            """
            [profiles.bad]
            width = 10
            height = 10
            clients = 2
            freq = 50
            [[profiles.bad.teams]]
            name = "a"
            ai = 5
            """
        )
        with self.assertRaisesRegex(ProfileError, "exceeds clients"):
            load_profiles(path)

    def test_duplicate_team_names(self):
        path = self.write(
            """
            [profiles.bad]
            width = 10
            height = 10
            clients = 4
            freq = 50
            [[profiles.bad.teams]]
            name = "a"
            ai = 1
            [[profiles.bad.teams]]
            name = "a"
            ai = 1
            """
        )
        with self.assertRaisesRegex(ProfileError, "duplicate team name"):
            load_profiles(path)


if __name__ == "__main__":
    unittest.main()
