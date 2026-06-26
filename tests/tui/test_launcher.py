##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test profile launcher
##

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../tui/src"))

from supervisor.commands import Binaries
from supervisor.launcher import attach_ai, attach_gui, launch_profile
from supervisor.profiles import Profile, Team
from supervisor.supervisor import Supervisor

# stub binaries: "true" ignores the zappy flags and exits cleanly, so we can
# exercise the spawning wiring without the real executables.
STUBS = Binaries(server="true", ai="true", gui="true")


def _profile(auto_gui, teams):
    return Profile(
        name="t",
        width=20,
        height=10,
        clients=6,
        freq=100,
        auto_gui=auto_gui,
        teams=teams,
    )


class TestLauncher(unittest.TestCase):
    def test_spawns_server_ais_and_gui(self):
        profile = _profile(True, (Team("red", 2), Team("blue", 1)))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            self.assertEqual(len(launch.ais), 3)
            self.assertEqual(len(launch.guis), 1)
            # 1 server + 3 ais + 1 gui, all tracked
            self.assertEqual(len(sup.processes), 5)

    def test_no_gui_when_not_requested(self):
        profile = _profile(False, (Team("red", 1),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            self.assertEqual(launch.guis, [])
            self.assertEqual(len(sup.processes), 2)  # server + 1 ai

    def test_ai_names_are_per_team_indexed(self):
        profile = _profile(False, (Team("red", 2), Team("blue", 1)))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            self.assertEqual(
                [ai.name for ai in launch.ais], ["red-0", "red-1", "blue-0"]
            )

    def test_all_share_the_allocated_port(self):
        profile = _profile(True, (Team("red", 1),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            for proc in (launch.server, *launch.ais, *launch.guis):
                self.assertIn(str(launch.port), proc.command)

    def test_team_strategy_passed_to_ai(self):
        profile = _profile(False, (Team("red", 1, "uai"),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            self.assertIn("-s", launch.ais[0].command)
            self.assertIn("uai", launch.ais[0].command)

    def test_zero_ai_team_spawns_no_ai(self):
        profile = _profile(False, (Team("red", 0),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            self.assertEqual(launch.ais, [])
            self.assertEqual(len(sup.processes), 1)  # server only

    def test_attach_ai_appends_and_names_next_index(self):
        profile = _profile(False, (Team("red", 1),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            ai = attach_ai(sup, launch, "red")
            self.assertEqual(ai.name, "red-1")  # red-0 already there
            self.assertEqual([a.name for a in launch.ais], ["red-0", "red-1"])
            self.assertIn(str(launch.port), ai.command)

    def test_attach_ai_passes_strategy(self):
        profile = _profile(False, (Team("red", 0),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)
            ai = attach_ai(sup, launch, "red", "uai")
            self.assertIn("-s", ai.command)
            self.assertIn("uai", ai.command)

    def test_attach_gui_stacks(self):
        profile = _profile(True, (Team("red", 0),))
        with Supervisor() as sup:
            launch = launch_profile(sup, profile, STUBS)  # one gui already
            second = attach_gui(sup, launch)
            self.assertEqual(second.name, "gui-1")
            self.assertEqual([g.name for g in launch.guis], ["gui-0", "gui-1"])
            self.assertIn(str(launch.port), second.command)


if __name__ == "__main__":
    unittest.main()
