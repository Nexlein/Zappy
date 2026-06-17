##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test states
##

import unittest
from context import DroneContext, Tile
from fsm.states.survival import ForageFood
from fsm.states.evolution import SearchStone
from fsm.states.swarm import BroadcastHelp, MapsToAlly
from context import BroadcastMessage
from fsm.states.evolution import IncantationState
from BroadcastProtocol import DecodedBroadcast, MessageType


def _rally(team: str, level: int, direction: int = 1) -> BroadcastMessage:
    """Build a pre-decoded RALLY message (decoding happens in the orchestrator)."""
    return BroadcastMessage(
        direction=direction,
        content=DecodedBroadcast(team, MessageType.RALLY, level),
    )


class TestForageFood(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = ForageFood()
        self.state.enter(self.context)

    def test_enter(self):
        self.state.enter(self.context)
        self.assertEqual(self.state._forward_streak, 0)

    def test_update_food_low(self):
        self.context.inventory.food = 5
        res = self.state.update(self.context)
        self.assertIsNone(res)

    def test_update_food_high(self):
        self.context.inventory.food = 15
        res = self.state.update(self.context)
        self.assertEqual(res, "SearchStone")

    def test_get_action_empty_vision(self):
        self.context.vision = []
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Look")

    def test_get_action_food_on_current_tile(self):
        self.context.vision = [Tile(food=2), Tile(food=0)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Take food")

    def test_get_action_no_food_on_current_tile(self):
        self.context.vision = [Tile(food=0), Tile(food=3)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Forward")

    def test_exit(self):
        res = self.state.exit(self.context)
        self.assertIsNone(res)


class TestSearchStone(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = SearchStone()
        # Default food to safe level
        self.context.inventory.food = 10
        self.state.enter(self.context)

    def test_enter(self):
        res = self.state.enter(self.context)
        self.assertIsNone(res)

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
        self.assertEqual(res, "BroadcastHelp")

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

    def test_get_action_ignore_non_missing_stone(self):
        self.context.level = 1
        self.context.inventory.linemate = 0  # Missing linemate
        # Current tile has sibur, but we don't need it at lvl 1
        self.context.vision = [Tile(sibur=1), Tile(linemate=1)]
        res = self.state.get_action(self.context)
        self.assertEqual(res, "Forward")

    def test_update_hear_ally_rally(self):
        self.context.level = 2
        self.context.team_name = "team5"
        self.context.broadcasts = [_rally("team5", 2)]
        res = self.state.update(self.context)
        self.assertEqual(res, "MapsToAlly")

    def test_update_ignore_wrong_level_rally(self):
        # Team filtering now happens in the orchestrator; states only
        # check the message type and level.
        self.context.level = 2
        self.context.team_name = "team5"
        self.context.broadcasts = [_rally("team5", 3)]
        res = self.state.update(self.context)
        self.assertNotEqual(res, "MapsToAlly")

    def test_exit(self):
        res = self.state.exit(self.context)
        self.assertIsNone(res)


class TestIncantationState(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = IncantationState()

    def test_enter(self):
        self.state.enter(self.context)
        self.assertFalse(self.state.command_sent)

    def test_get_action(self):
        self.state.enter(self.context)
        action = self.state.get_action(self.context)
        self.assertEqual(action, "Incantation")
        self.assertTrue(self.state.command_sent)
        action2 = self.state.get_action(self.context)
        self.assertIsNone(action2)

    def test_update_success(self):
        self.state.enter(self.context)
        self.state.get_action(self.context)
        self.context.last_command_successful = True
        self.context.level = 2
        res = self.state.update(self.context)
        self.assertEqual(res, "SearchStone")

    def test_update_failure(self):
        self.state.enter(self.context)
        self.state.get_action(self.context)
        self.context.last_command_successful = False
        res = self.state.update(self.context)
        self.assertEqual(res, "SearchStone")


class TestBroadcastHelp(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = BroadcastHelp()

    def test_enter(self):
        self.state.enter(self.context)
        self.assertEqual(self.state.ticks_waited, 0)

    def test_update_survival_trigger(self):
        self.state.enter(self.context)
        self.context.inventory.food = 2  # Less than SURVIVAL_THRESHOLD (5)
        res = self.state.update(self.context)
        self.assertIsNone(res)
        self.assertEqual(self.state._abort_target, "ForageFood")

    def test_update_timeout(self):
        self.state.enter(self.context)
        self.context.inventory.food = 15  # Food secure
        self.state.ticks_waited = 301  # Greater than RALLY_TIMEOUT (300)
        res = self.state.update(self.context)
        self.assertIsNone(res)
        self.assertEqual(self.state._abort_target, "Reproduce")

    def test_update_incantation_ready(self):
        self.state.enter(self.context)
        self.context.inventory.food = 15  # Food secure
        self.context.vision = [Tile(player=2, linemate=1, deraumere=1, sibur=1)]
        self.context.level = 2
        self.state.ready_count = 1  # Faking 1 teammate ready
        res = self.state.update(self.context)
        self.assertEqual(res, "Incantation")


class TestMapsToAlly(unittest.TestCase):
    def setUp(self):
        self.context = DroneContext()
        self.state = MapsToAlly()

    def test_enter(self):
        self.state.enter(self.context)
        self.assertEqual(self.state.ticks_waited, 0)
        self.assertFalse(self.state.arrived)

    def test_update_survival_trigger(self):
        self.state.enter(self.context)
        self.context.inventory.food = 2  # Less than SURVIVAL_THRESHOLD (5)
        res = self.state.update(self.context)
        self.assertEqual(res, "ForageFood")

    def test_update_timeout(self):
        self.state.enter(self.context)
        self.context.inventory.food = 15  # Food secure
        self.state.ticks_waited = 301  # Greater than RALLY_TIMEOUT (300)
        res = self.state.update(self.context)
        self.assertEqual(res, "SearchStone")

    def test_get_action_follow_broadcast(self):
        self.state.enter(self.context)
        self.context.level = 1
        self.context.team_name = "team5"
        self.context.broadcasts = [_rally("team5", 1)]
        action = self.state.get_action(self.context)
        self.assertEqual(action, "Forward")
