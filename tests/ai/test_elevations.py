##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## test_elevations
##

import sys
import os
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from context import Tile
from elevations import is_incantation_ready


class TestElevations(unittest.TestCase):
    def test_level_1_ready(self):
        tile = Tile(player=1, linemate=1)
        self.assertTrue(is_incantation_ready(1, tile))

    def test_level_1_not_enough_players(self):
        tile = Tile(player=0, linemate=1)
        self.assertFalse(is_incantation_ready(1, tile))

    def test_level_1_missing_stones(self):
        tile = Tile(player=1, linemate=0)
        self.assertFalse(is_incantation_ready(1, tile))

    def test_level_2_ready(self):
        tile = Tile(player=2, linemate=1, deraumere=1, sibur=1)
        self.assertTrue(is_incantation_ready(2, tile))

    def test_level_2_not_enough_players(self):
        tile = Tile(player=1, linemate=1, deraumere=1, sibur=1)
        self.assertFalse(is_incantation_ready(2, tile))

    def test_level_2_missing_stones(self):
        tile = Tile(player=2, linemate=1, deraumere=0, sibur=1)
        self.assertFalse(is_incantation_ready(2, tile))


if __name__ == "__main__":
    unittest.main()
