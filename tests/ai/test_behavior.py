##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test Behavior
##

import unittest
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

from context import DroneContext, Tile
from fsm.controller import AIController


class TestBehavior(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.controller = AIController(self.context)

    def test_forage_behavior_sequence(self):
        """
        Simulates a drone acting over a few ticks to verify sequence:
        Tick 1: Has no vision -> "Look"
        Tick 2: Has vision, sees food -> pathfinds -> "Forward"
        Tick 3: Follows path -> "Forward"
        Tick 4: Reaches target tile -> "Take food"
        """
        # Drone starts with low food, defaults to ForageFood state
        self.context.inventory.food = 3

        # Tick 1: No vision loaded yet.
        self.context.vision = []
        action1 = self.controller.tick()
        self.assertEqual(action1, "Look")

        # Mock the server response: we see food on tile index 6 (Forward -> Forward)
        self.context.vision = [Tile()] * 6 + [Tile(food=1)]

        # Tick 2: Should queue path (Forward, Forward) and return first element
        action2 = self.controller.tick()
        self.assertEqual(action2, "Forward")

        # In a real game, 'Forward' moves the drone. The orchestrator clears vision on move.
        self.context.vision.clear()

        # Tick 3: The drone has a path queued, it should output the next action
        action3 = self.controller.tick()
        self.assertEqual(action3, "Forward")

        # The drone is now on the tile that had food.
        # It's out of queued movements, so it will evaluate survival action again.
        # But wait, since vision is clear, it will Look again unless we mock the look.
        # Let's mock a Look response after moving:
        self.context.vision = [Tile(food=1)]

        # Tick 4: Sees food on its current tile
        action4 = self.controller.tick()
        self.assertEqual(action4, "Take food")

        # Tick 5: Simulate success, food increases.
        self.context.inventory.food += 1
        self.context.vision.clear()

        # Food is 4, still below threshold, it will look again
        action5 = self.controller.tick()
        self.assertEqual(action5, "Look")

    def test_elevation_sequence(self):
        """
        Simulate picking up stones and attempting elevation at level 1.
        """
        # Start with safe food to trigger SearchStone
        self.context.inventory.food = 20
        self.context.level = 1

        # We need 1 linemate to elevate to level 2.
        self.context.inventory.linemate = 0

        # Run tick to transition from ForageFood -> SearchStone
        self.controller.tick()
        self.assertEqual(self.controller.current_state_name, "SearchStone")

        # Tick 1: No vision -> Look
        self.context.vision = []
        self.assertEqual(self.controller.tick(), "Look")

        # Tick 2: See linemate on current tile
        self.context.vision = [Tile(linemate=1)]
        self.assertEqual(self.controller.tick(), "Take linemate")

        # Simulate success
        self.context.inventory.linemate += 1
        self.context.vision.clear()

        # Next tick, SearchStone will evaluate, see we have enough stones, and transition to BroadcastHelp.
        self.controller.tick()
        self.assertEqual(self.controller.current_state_name, "BroadcastHelp")

        # Next tick, BroadcastHelp sees we are level 1 (only 1 player needed) and is ready.
        self.context.vision = [Tile(player=1, linemate=1)]
        self.controller.tick()
        self.assertEqual(self.controller.current_state_name, "Incantation")


if __name__ == "__main__":
    unittest.main()
