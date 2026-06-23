##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test look_parser
##

import unittest
from context import Tile
from protocol.look_parser import _parse_look, parse_look_to_tiles


class TestLookParser(unittest.TestCase):
    def test_invalid_formats(self):
        with self.assertRaises(ValueError):
            _parse_look("player, food")  # No brackets
        with self.assertRaises(ValueError):
            _parse_look("[player, food")  # No closing bracket
        with self.assertRaises(ValueError):
            _parse_look("player, food]")  # No opening bracket

    def test_empty_string(self):
        self.assertEqual(_parse_look("[]"), [])
        self.assertEqual(_parse_look("[   ]"), [])

    def test_single_tile_player(self):
        res = _parse_look("[player]")
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0]["index"], 0)
        self.assertEqual(res[0]["x"], 0)
        self.assertEqual(res[0]["y"], 0)
        self.assertEqual(res[0]["player"], 1)
        self.assertEqual(res[0]["food"], 0)

    def test_multiple_items_on_tile(self):
        res = _parse_look("[player player food thystame]")
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0]["player"], 2)
        self.assertEqual(res[0]["food"], 1)
        self.assertEqual(res[0]["thystame"], 1)
        self.assertEqual(res[0]["linemate"], 0)

    def test_level_1_cone(self):
        # Level 1 has 4 tiles (indices 0 to 3)
        look_str = "[player, food, linemate sibur, player deraumere]"
        res = _parse_look(look_str)
        self.assertEqual(len(res), 4)

        # Tile 0: player
        self.assertEqual(res[0]["index"], 0)
        self.assertEqual(res[0]["x"], 0)
        self.assertEqual(res[0]["y"], 0)
        self.assertEqual(res[0]["player"], 1)

        # Tile 1: food (relative left)
        self.assertEqual(res[1]["index"], 1)
        self.assertEqual(res[1]["x"], -1)
        self.assertEqual(res[1]["y"], 1)
        self.assertEqual(res[1]["food"], 1)

        # Tile 2: linemate sibur (relative center)
        self.assertEqual(res[2]["index"], 2)
        self.assertEqual(res[2]["x"], 0)
        self.assertEqual(res[2]["y"], 1)
        self.assertEqual(res[2]["linemate"], 1)
        self.assertEqual(res[2]["sibur"], 1)

        # Tile 3: player deraumere (relative right)
        self.assertEqual(res[3]["index"], 3)
        self.assertEqual(res[3]["x"], 1)
        self.assertEqual(res[3]["y"], 1)
        self.assertEqual(res[3]["player"], 1)
        self.assertEqual(res[3]["deraumere"], 1)

    def test_parse_to_tiles(self):
        look_str = "[player, food,, linemate]"
        tiles = parse_look_to_tiles(look_str)
        self.assertEqual(len(tiles), 4)
        self.assertIsInstance(tiles[0], Tile)
        self.assertEqual(tiles[0].player, 1)
        self.assertEqual(tiles[1].food, 1)
        self.assertEqual(tiles[2].food, 0)
        self.assertEqual(tiles[3].linemate, 1)

    def test_clean_and_case_insensitive(self):
        look_str = "[  PlAyEr   ,   FoOd  LinEmAtE  ]"
        res = _parse_look(look_str)
        self.assertEqual(len(res), 2)
        self.assertEqual(res[0]["player"], 1)
        self.assertEqual(res[1]["food"], 1)
        self.assertEqual(res[1]["linemate"], 1)
