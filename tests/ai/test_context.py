##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test context
##

import unittest
from context import Inventory, Tile, BroadcastMessage, DroneContext


class TestContext(unittest.TestCase):
    def test_inventory_defaults(self):
        inv = Inventory()
        self.assertEqual(inv.food, 0)
        self.assertEqual(inv.linemate, 0)
        self.assertEqual(inv.deraumere, 0)
        self.assertEqual(inv.sibur, 0)
        self.assertEqual(inv.mendiane, 0)
        self.assertEqual(inv.phiras, 0)
        self.assertEqual(inv.thystame, 0)

    def test_inventory_custom(self):
        inv = Inventory(food=10, linemate=2, deraumere=1)
        self.assertEqual(inv.food, 10)
        self.assertEqual(inv.linemate, 2)
        self.assertEqual(inv.deraumere, 1)
        self.assertEqual(inv.sibur, 0)

    def test_tile_defaults(self):
        tile = Tile()
        self.assertEqual(tile.player, 0)
        self.assertEqual(tile.food, 0)
        self.assertEqual(tile.linemate, 0)
        self.assertEqual(tile.deraumere, 0)
        self.assertEqual(tile.sibur, 0)
        self.assertEqual(tile.mendiane, 0)
        self.assertEqual(tile.phiras, 0)
        self.assertEqual(tile.thystame, 0)

    def test_broadcast_message(self):
        msg = BroadcastMessage(direction=4, text="message")
        self.assertEqual(msg.direction, 4)
        self.assertEqual(msg.text, "message")

    def test_drone_context_defaults(self):
        ctx = DroneContext()
        self.assertEqual(ctx.team_name, "")
        self.assertEqual(ctx.map_width, 0)
        self.assertEqual(ctx.map_height, 0)
        self.assertEqual(ctx.available_slots, 0)
        self.assertEqual(ctx.level, 1)
        self.assertIsInstance(ctx.inventory, Inventory)
        self.assertEqual(ctx.vision, [])
        self.assertEqual(ctx.broadcasts, [])
        self.assertIsNone(ctx.last_command_successful)
