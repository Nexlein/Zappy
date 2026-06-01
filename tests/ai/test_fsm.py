##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test FSM controller
##

import unittest
from unittest.mock import MagicMock
from context import DroneContext
from fsm import AIController
from states.AStates import State


class TestAIController(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.controller = AIController(self.context)

    def test_initialization(self):
        self.assertEqual(self.controller.current_state_name, "ForageFood")
        self.assertIsNotNone(self.controller.current_state)

    def test_tick_returns_action(self):
        # By default, ForageFood get_action on empty vision returns "Look"
        self.context.vision = []
        action = self.controller.tick()
        self.assertEqual(action, "Look")

    def test_transition_to(self):
        # Create a mock state to transition to
        mock_state = MagicMock(spec=State)
        self.controller.states["MockState"] = mock_state

        # Mock the current state's exit method
        current_state_mock = MagicMock(spec=State)
        self.controller.current_state = current_state_mock

        # Transition to MockState
        self.controller._transition_to("MockState")

        # Verify exit was called on previous state
        current_state_mock.exit.assert_called_once_with(self.context)
        # Verify enter was called on new state
        mock_state.enter.assert_called_once_with(self.context)
        self.assertEqual(self.controller.current_state_name, "MockState")
        self.assertEqual(self.controller.current_state, mock_state)

    def test_tick_handles_transition(self):
        # Setup current state to return "NextState" on update
        current_state_mock = MagicMock(spec=State)
        current_state_mock.update.return_value = "NextState"
        self.controller.current_state = current_state_mock
        self.controller.current_state_name = "ForageFood"

        # Setup next state
        next_state_mock = MagicMock(spec=State)
        next_state_mock.get_action.return_value = "MockAction"
        self.controller.states["NextState"] = next_state_mock

        # Run tick
        action = self.controller.tick()

        # Verify exit was called on current
        current_state_mock.exit.assert_called_once_with(self.context)
        # Verify enter was called on next state
        next_state_mock.enter.assert_called_once_with(self.context)
        # Verify get_action was called on next state
        next_state_mock.get_action.assert_called_once_with(self.context)
        self.assertEqual(action, "MockAction")
