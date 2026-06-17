##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test Utility-based AI Controller
##

import unittest
from context import DroneContext, Tile, BroadcastMessage
from uai.controller import UtilityAIController
from BroadcastProtocol import BroadcastProtocol


class TestUtilityAIController(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.context.ticks_since_inventory = 0
        self.controller = UtilityAIController(self.context)

    def test_initialization(self):
        self.assertEqual(self.controller.context, self.context)
        self.assertEqual(self.controller._forward_streak, 0)

    def test_tick_triggers_inventory_refresh(self):
        self.context.ticks_since_inventory = 15
        action = self.controller.tick()
        self.assertEqual(action, "Inventory")
        self.assertEqual(self.context.ticks_since_inventory, 0)

    def test_survival_utility(self):
        # When food is below threshold, survival utility is 1.0
        self.context.inventory.food = 2
        u_surv = self.controller._get_survival_utility()
        self.assertEqual(u_surv, 1.0)

        # When food is above target, survival utility is 0.0
        self.context.inventory.food = 20
        u_surv = self.controller._get_survival_utility()
        self.assertEqual(u_surv, 0.0)

    def test_tick_takes_food_when_low(self):
        self.context.inventory.food = 3  # Low food
        self.context.vision = [Tile(food=2)]
        action = self.controller.tick()
        self.assertEqual(action, "Take food")

    def test_tick_looks_for_food_when_none_on_tile(self):
        self.context.inventory.food = 3  # Low food
        self.context.vision = [Tile(food=0)]
        action = self.controller.tick()
        self.assertEqual(action, "Forward")

    def test_gather_utility_and_action(self):
        self.context.inventory.food = 15  # Safe food
        self.controller.forks_done = 10  # Max forks done, so reproduce utility is 0
        self.context.level = 1
        self.context.inventory.linemate = 0  # Missing linemate
        self.context.vision = [Tile(linemate=1)]

        # Should choose gather behavior and return Take linemate
        action = self.controller.tick()
        self.assertEqual(action, "Take linemate")

    def test_incantation_utility_and_action(self):
        self.context.inventory.food = 15  # Safe food
        self.context.level = 1
        self.context.inventory.linemate = 1
        self.context.vision = [Tile(player=1, linemate=1)]

        # Should choose incantation and return Incantation
        action = self.controller.tick()
        self.assertEqual(action, "Incantation")

    def test_follow_utility_and_action(self):
        self.context.inventory.food = 12  # Safe food
        self.context.level = 2
        self.context.team_name = "test_team"
        self.context.inventory.linemate = 0  # Missing linemate
        self.context.broadcasts = [
            BroadcastMessage(
                direction=1, content=BroadcastProtocol.decode("test_team|RALLY|2")
            )
        ]

        # Should follow teammate
        action = self.controller.tick()
        self.assertEqual(action, "Forward")
