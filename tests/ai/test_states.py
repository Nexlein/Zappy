##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test states
##

import unittest
from context import DroneContext, Tile
from states.survival import ForageFood


class TestForageFood(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = ForageFood()

    def test_enter(self):
        res = self.state.enter(self.context)
        self.assertEqual(res, "Look")

    def test_update_food_low(self):
        self.context.inventory.food = 5
        res = self.state.update(self.context)
        self.assertIsNone(res)

    def test_update_food_high(self):
        self.context.inventory.food = 15
        res = self.state.update(self.context)
        self.assertIsNone(res)

    def test_get_action_empty_vision(self):
        self.context.vision = []
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Look")

    def test_get_action_food_on_current_tile(self):
        self.context.vision = [Tile(food=2), Tile(food=0)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Take food")
        # Vision should be cleared
        self.assertEqual(self.context.vision, [])

    def test_get_action_no_food_on_current_tile(self):
        self.context.vision = [Tile(food=0), Tile(food=3)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Forward")
        # Vision should be cleared
        self.assertEqual(self.context.vision, [])

    def test_exit(self):
        res = self.state.exit(self.context)
        self.assertIsNone(res)
