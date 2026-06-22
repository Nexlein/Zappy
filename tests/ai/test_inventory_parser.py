##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test inventory_parser
##

import unittest
from context import Inventory
from protocol.inventory_parser import _parse_inventory, update_inventory


class TestInventoryParser(unittest.TestCase):
    def test_invalid_formats(self):
        with self.assertRaises(ValueError):
            _parse_inventory("food 345, sibur 3")  # No brackets
        with self.assertRaises(ValueError):
            _parse_inventory("[food 345, sibur 3")  # No closing bracket
        with self.assertRaises(ValueError):
            _parse_inventory("food 345, sibur 3]")  # No opening bracket
        with self.assertRaises(ValueError):
            _parse_inventory("[food, sibur 3]")  # Missing count
        with self.assertRaises(ValueError):
            _parse_inventory("[food abc, sibur 3]")  # Invalid non-integer count

    def test_empty_inventory(self):
        self.assertEqual(_parse_inventory("[]"), {})
        self.assertEqual(_parse_inventory("[   ]"), {})

    def test_valid_parsing(self):
        inv_str = "[food 345, sibur 3, phiras 5, linemate 0, thystame 12]"
        res = _parse_inventory(inv_str)
        self.assertEqual(res["food"], 345)
        self.assertEqual(res["sibur"], 3)
        self.assertEqual(res["phiras"], 5)
        self.assertEqual(res["linemate"], 0)
        self.assertEqual(res["thystame"], 12)

    def test_whitespace_and_case_insensitive(self):
        inv_str = "[  FoOd   345 ,   sIbUr   3  ]"
        res = _parse_inventory(inv_str)
        self.assertEqual(res["food"], 345)
        self.assertEqual(res["sibur"], 3)

    def test_update_inventory(self):
        inv = Inventory()
        # Pre-check defaults
        self.assertEqual(inv.food, 0)
        self.assertEqual(inv.sibur, 0)

        inv_str = "[food 15, sibur 2, deraumere 4]"
        update_inventory(inv, inv_str)

        self.assertEqual(inv.food, 15)
        self.assertEqual(inv.sibur, 2)
        self.assertEqual(inv.deraumere, 4)
        self.assertEqual(inv.linemate, 0)  # Unmodified remains 0
