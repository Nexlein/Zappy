##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test Navigation
##

import unittest
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from utils.navigation import get_action_for_broadcast


class TestNavigation(unittest.TestCase):
    def test_get_action_arrived(self):
        self.assertIsNone(get_action_for_broadcast(0))

    def test_get_action_forward(self):
        for direction in [1, 2, 8]:
            self.assertEqual(get_action_for_broadcast(direction), "Forward")

    def test_get_action_right(self):
        for direction in [6, 7]:
            self.assertEqual(get_action_for_broadcast(direction), "Right")

    def test_get_action_left(self):
        for direction in [3, 4, 5]:
            self.assertEqual(get_action_for_broadcast(direction), "Left")

    def test_get_action_fallback(self):
        # Invalid directions should fallback to Forward
        self.assertEqual(get_action_for_broadcast(9), "Forward")
        self.assertEqual(get_action_for_broadcast(-1), "Forward")


if __name__ == "__main__":
    unittest.main()
