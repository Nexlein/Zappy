##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## Test Config Loader
##

import unittest
import sys
import os
import json
from unittest.mock import patch, mock_open

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "../../ai/src"))

import utils.config_loader as cl


class TestConfigLoader(unittest.TestCase):
    def setUp(self):
        # Reset the global cache before each test
        cl._CONFIG_CACHE = None

    def tearDown(self):
        # Reset the global cache after each test to avoid bleeding into other tests
        cl._CONFIG_CACHE = None

    def test_get_config_success(self):
        mock_data = '{"survival": {"SURVIVAL_THRESHOLD": 5}}'
        with patch("builtins.open", mock_open(read_data=mock_data)):
            config = cl.get_config()
            self.assertEqual(config, {"survival": {"SURVIVAL_THRESHOLD": 5}})

    def test_get_config_cache(self):
        # First load
        mock_data = '{"survival": {"SURVIVAL_THRESHOLD": 5}}'
        with patch("builtins.open", mock_open(read_data=mock_data)) as mocked_file:
            config1 = cl.get_config()
            config2 = cl.get_config()
            # File should only be opened once
            mocked_file.assert_called_once()
            self.assertEqual(config1, config2)

    def test_get_config_fallback_on_error(self):
        # Simulate FileNotFoundError
        with patch("builtins.open", side_effect=FileNotFoundError):
            config = cl.get_config()
            self.assertEqual(config, {})

    def test_get_config_fallback_on_invalid_json(self):
        # Simulate invalid json
        mock_data = '{"survival": {"SURVIVAL_THRESHOLD": 5'
        with patch("builtins.open", mock_open(read_data=mock_data)):
            config = cl.get_config()
            self.assertEqual(config, {})

    def test_get_specific_configs(self):
        mock_data = json.dumps(
            {
                "survival": {"FOOD_TARGET": 20},
                "evolution": {"MAX_LEVEL": 8},
                "reproduction": {"FORK_FOOD_THRESHOLD": 10},
                "swarm": {"RALLY_TIMEOUT": 100},
                "network": {"TCP_RECV_BUFFER_SIZE": 8192},
            }
        )
        with patch("builtins.open", mock_open(read_data=mock_data)):
            self.assertEqual(cl.get_survival_config(), {"FOOD_TARGET": 20})
            self.assertEqual(cl.get_evolution_config(), {"MAX_LEVEL": 8})
            self.assertEqual(cl.get_reproduction_config(), {"FORK_FOOD_THRESHOLD": 10})
            self.assertEqual(cl.get_swarm_config(), {"RALLY_TIMEOUT": 100})
            self.assertEqual(cl.get_network_config(), {"TCP_RECV_BUFFER_SIZE": 8192})


if __name__ == "__main__":
    unittest.main()
