import json
import os
from typing import Any

_CONFIG_CACHE: dict[str, Any] | None = None


def get_config() -> dict[str, Any]:
    global _CONFIG_CACHE
    if _CONFIG_CACHE is not None:
        return _CONFIG_CACHE

    config_path = os.path.join(
        os.path.dirname(__file__), "..", "..", "config", "config.json"
    )
    try:
        with open(config_path, "r") as f:
            _CONFIG_CACHE = json.load(f)
    except Exception:
        # Fallback to absolute bare minimum defaults if file somehow fails
        _CONFIG_CACHE = {}
    return _CONFIG_CACHE if _CONFIG_CACHE is not None else {}


def get_survival_config() -> dict[str, Any]:
    return get_config().get("survival", {})


def get_evolution_config() -> dict[str, Any]:
    return get_config().get("evolution", {})


def get_reproduction_config() -> dict[str, Any]:
    return get_config().get("reproduction", {})


def get_swarm_config() -> dict[str, Any]:
    return get_config().get("swarm", {})


def get_network_config() -> dict[str, Any]:
    return get_config().get("network", {})


def get_client_config() -> dict[str, Any]:
    return get_config().get("client", {})
