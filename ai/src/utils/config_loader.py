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


# Average map dimension the base config thresholds were tuned for (10x10).
_MAP_SCALE_REF = 10
# Extra food a drone carries per unit of average map dimension above the ref.
_FOOD_TARGET_PER_SIZE = 1.5


def apply_map_scaling(map_width: int, map_height: int):
    """Scale survival food thresholds to map size and return before/after."""
    keys = (
        "SAFE_FOOD_THRESHOLD",
        "FOOD_TARGET",
        "FOOD_CEILING",
    )
    surv = get_config().get("survival", {})
    before = {k: surv.get(k) for k in keys}

    avg_size = (map_width + map_height) / 2
    growth = max(0.0, avg_size - _MAP_SCALE_REF)

    base_target = before["FOOD_TARGET"] or 25
    base_safe = before["SAFE_FOOD_THRESHOLD"] or 15
    safe_ratio = base_safe / base_target if base_target else 0.6

    food_target = int(round(base_target + _FOOD_TARGET_PER_SIZE * growth))
    surv["FOOD_TARGET"] = food_target
    surv["SAFE_FOOD_THRESHOLD"] = int(round(food_target * safe_ratio))
    surv["FOOD_CEILING"] = max(before["FOOD_CEILING"] or 45, food_target + 10)


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
