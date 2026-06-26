##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test game manager
##

import os
import stat
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.commands import Binaries
from supervisor.manager import ZappyManager
from supervisor.profiles import Profile, Team
from supervisor.supervisor import Supervisor

# a stub binary that ignores the zappy flags and stays alive, so kill/stop tests
# can observe which children are still running.
_STUB_DIR = tempfile.mkdtemp()
_STUB = os.path.join(_STUB_DIR, "stub")
with open(_STUB, "w") as _f:
    _f.write("#!/bin/sh\nexec sleep 30\n")
os.chmod(_STUB, os.stat(_STUB).st_mode | stat.S_IEXEC)

STUBS = Binaries(server=_STUB, ai=_STUB, gui=_STUB)


def _profile(auto_gui, teams, name="t"):
    # sleep ignores zappy flags; args end with the port number -> a valid delay
    return Profile(
        name=name,
        width=20,
        height=10,
        clients=6,
        freq=30,
        auto_gui=auto_gui,
        teams=teams,
    )


class TestManager(unittest.TestCase):
    def _manager(self, sup):
        return ZappyManager(sup, binaries=STUBS)

    def test_launch_registers_game(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("duel", _profile(False, (Team("red", 1),)))
            self.assertEqual(mgr.games, [game])
            self.assertEqual(game.name, "duel")

    def test_attach_ai_to_named_team(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, (Team("red", 1), Team("blue", 1))))
            ai = mgr.attach_ai(game, "blue")
            self.assertEqual(ai.name, "blue-1")

    def test_attach_ai_with_strategy(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, (Team("red", 0),)))
            ai = mgr.attach_ai(game, "red", "uai")
            self.assertIn("uai", ai.command)

    def test_attach_ai_no_strategy_omits_flag(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, (Team("red", 0),)))
            ai = mgr.attach_ai(game, "red")
            self.assertNotIn("-s", ai.command)

    def test_default_team_from_near_ai(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, (Team("red", 1), Team("blue", 1))))
            blue0 = next(a for a in game.launch.ais if a.name == "blue-0")
            self.assertEqual(mgr.default_team(game, near=blue0), "blue")

    def test_default_team_falls_back_to_first(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, (Team("red", 0), Team("blue", 0))))
            self.assertEqual(mgr.default_team(game), "red")

    def test_default_team_none_without_teams(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, ()))
            self.assertIsNone(mgr.default_team(game))

    def test_team_names_and_strategy_of(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch(
                "g", _profile(False, (Team("red", 0, "queen"), Team("blue", 0)))
            )
            self.assertEqual(mgr.team_names(game), ["red", "blue"])
            self.assertEqual(mgr.strategy_of(game, "red"), "queen")
            self.assertIsNone(mgr.strategy_of(game, "blue"))

    def test_attach_gui_stacks(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(True, (Team("red", 0),)))
            second = mgr.attach_gui(game)
            self.assertEqual([g.name for g in game.launch.guis], ["gui-0", "gui-1"])
            self.assertEqual(second.name, "gui-1")

    def test_kill_one_keeps_others(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(False, (Team("red", 1),)))
            ai = game.launch.ais[0]
            mgr.kill(ai)
            self.assertFalse(ai.is_alive())
            self.assertTrue(game.launch.server.is_alive())

    def test_stop_game_kills_all_members(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            game = mgr.launch("g", _profile(True, (Team("red", 2),)))
            mgr.stop_game(game)
            members = [game.launch.server, *game.launch.ais, *game.launch.guis]
            self.assertTrue(all(not m.is_alive() for m in members))

    def test_game_of_finds_owner(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            a = mgr.launch("a", _profile(False, (Team("red", 1),)))
            b = mgr.launch("b", _profile(False, (Team("blue", 1),)))
            self.assertIs(mgr.game_of(a.launch.ais[0]), a)
            self.assertIs(mgr.game_of(b.launch.server), b)

    def test_game_of_none_for_unknown(self):
        with Supervisor() as sup:
            mgr = self._manager(sup)
            mgr.launch("a", _profile(False, (Team("red", 1),)))
            self.assertIsNone(mgr.game_of(None))


if __name__ == "__main__":
    unittest.main()
