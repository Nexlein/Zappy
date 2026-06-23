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


def _deep_update(d, u):
    for k, v in u.items():
        if isinstance(v, dict):
            d[k] = _deep_update(d.get(k, {}), v)
        else:
            d[k] = v
    return d


def load_strategy_config(strategy: str) -> None:
    global _CONFIG_CACHE
    get_config()  # Ensure base config is loaded

    strategy_config_path = os.path.join(
        os.path.dirname(__file__), "..", "..", "config", f"{strategy}_config.json"
    )
    if os.path.exists(strategy_config_path):
        try:
            with open(strategy_config_path, "r") as f:
                strategy_config = json.load(f)
            _CONFIG_CACHE = _deep_update(_CONFIG_CACHE, strategy_config)
        except Exception as e:
            print(f"Warning: Failed to load {strategy}_config.json: {e}")


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
