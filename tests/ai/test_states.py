##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test states
##

import unittest
from context import DroneContext, Tile
from states.survival import ForageFood
from states.evolution import SearchStone


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


class TestSearchStone(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = SearchStone()
        # Default food to safe level
        self.context.inventory.food = 10

    def test_enter(self):
        res = self.state.enter(self.context)
        self.assertEqual(res, "Look")

    def test_get_missing_stones_level_1_missing(self):
        self.context.level = 1
        self.context.inventory.linemate = 0
        missing = self.state._get_missing_stones(self.context)
        self.assertEqual(missing, {"linemate": 1})

    def test_get_missing_stones_level_1_complete(self):
        self.context.level = 1
        self.context.inventory.linemate = 1
        missing = self.state._get_missing_stones(self.context)
        self.assertEqual(missing, {})

    def test_update_low_food_trigger(self):
        self.context.inventory.food = 4
        res = self.state.update(self.context)
        self.assertEqual(res, "ForageFood")

    def test_update_all_stones_collected_level_1(self):
        self.context.level = 1
        self.context.inventory.linemate = 1
        res = self.state.update(self.context)
        self.assertEqual(res, "IncantationState")

    def test_update_all_stones_collected_level_2(self):
        self.context.level = 2
        self.context.inventory.linemate = 1
        self.context.inventory.deraumere = 1
        self.context.inventory.sibur = 1
        res = self.state.update(self.context)
        self.assertEqual(res, "BroadcastHelp")

    def test_update_still_missing_stones(self):
        self.context.level = 1
        self.context.inventory.linemate = 0
        res = self.state.update(self.context)
        self.assertIsNone(res)

    def test_get_action_empty_vision(self):
        self.context.vision = []
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Look")

    def test_get_action_take_missing_stone(self):
        self.context.level = 1
        self.context.inventory.linemate = 0  # Missing linemate
        self.context.vision = [Tile(linemate=1), Tile(linemate=0)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Take linemate")
        self.assertEqual(self.context.vision, [])

    def test_get_action_ignore_non_missing_stone(self):
        self.context.level = 1
        self.context.inventory.linemate = 0  # Missing linemate
        # Current tile has sibur, but we don't need it at lvl 1
        self.context.vision = [Tile(sibur=1), Tile(linemate=1)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Forward")
        self.assertEqual(self.context.vision, [])

    def test_exit(self):
        res = self.state.exit(self.context)
        self.assertIsNone(res)
