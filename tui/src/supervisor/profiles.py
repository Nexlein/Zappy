import tomllib
from dataclasses import dataclass
from pathlib import Path


class ProfileError(Exception):
    """Raised when a profile file is invalid, missing or malformed."""


@dataclass(frozen=True)
class Team:
    name: str
    ai: int


@dataclass(frozen=True)
class Profile:
    name: str
    width: int
    height: int
    clients: int
    freq: int
    auto_gui: bool
    teams: tuple[Team, ...]


_MISSING = object()


def load_profiles(path: Path) -> dict[str, Profile]:
    """Load and validate every profile in a TOML file. Keyed by name."""
    raw = _read_toml(path)

    profiles_table = raw.get("profiles")
    if not isinstance(profiles_table, dict) or not profiles_table:
        raise ProfileError(f"{path}: no [profiles.*] tables found")

    return {name: _build_profile(name, body) for name, body in profiles_table.items()}


def _read_toml(path: Path) -> dict:
    try:
        with path.open("rb") as f:
            return tomllib.load(f)
    except FileNotFoundError:
        raise ProfileError(f"profile file not found: {path}")
    except tomllib.TOMLDecodeError as e:
        raise ProfileError(f"{path}: invalid TOML: {e}") from e


_ALLOWED_KEYS = {"width", "height", "clients", "freq", "auto_gui", "teams"}


def _build_profile(name: str, body: dict) -> Profile:
    if not isinstance(body, dict):
        raise ProfileError(
            f"profile {name}: expected a table, got {type(body).__name__}"
        )

    unknown = set(body) - _ALLOWED_KEYS
    if unknown:
        raise ProfileError(f"profile {name}: unknown key(s) {sorted(unknown)}")

    def get(key: str, default=_MISSING):
        value = body.get(key, default)
        if value is _MISSING:
            raise ProfileError(f"profile {name}: missing required key '{key}'")
        return value

    def get_positive_int(key: str) -> int:
        value = get(key)
        if isinstance(value, bool) or not isinstance(value, int):
            raise ProfileError(
                f"profile {name}: '{key}' must be an integer, got {value!r}"
            )
        if value <= 0:
            raise ProfileError(f"profile {name}: '{key}' must be > 0, got {value}")
        return value

    def get_bool(key: str, default) -> bool:
        value = get(key, default)
        if not isinstance(value, bool):
            raise ProfileError(
                f"profile {name}: '{key}' must be true/false, got {value!r}"
            )
        return value

    width = get_positive_int("width")
    height = get_positive_int("height")
    clients = get_positive_int("clients")
    freq = get_positive_int("freq")
    auto_gui = get_bool("auto_gui", False)
    teams = _build_teams(name, get("teams"), clients)

    return Profile(name, width, height, clients, freq, auto_gui, teams)


def _build_teams(profile_name: str, raw, clients: int) -> tuple[Team, ...]:
    if not isinstance(raw, list) or not raw:
        raise ProfileError(
            f"profile {profile_name}: 'teams' must be a non-empty array of tables"
        )

    teams: list[Team] = []
    seen: set[str] = set()
    for index, entry in enumerate(raw):
        ctx = f"profile {profile_name}: team[{index}]"
        if not isinstance(entry, dict):
            raise ProfileError(f"{ctx}: expected a table, got {type(entry).__name__}")

        unknown = set(entry) - {"name", "ai"}
        if unknown:
            raise ProfileError(f"{ctx}: unknown key(s) {sorted(unknown)}")

        name = entry.get("name")
        if not isinstance(name, str) or not name.strip():
            raise ProfileError(f"{ctx}: 'name' must be a non-empty string")
        if name in seen:
            raise ProfileError(f"profile {profile_name}: duplicate team name {name!r}")
        seen.add(name)

        ai = entry.get("ai", _MISSING)
        if ai is _MISSING:
            raise ProfileError(f"{ctx}: missing required key 'ai'")
        if isinstance(ai, bool) or not isinstance(ai, int):
            raise ProfileError(f"{ctx}: 'ai' must be an integer, got {ai!r}")
        if ai < 0:
            raise ProfileError(f"{ctx}: 'ai' must be >= 0, got {ai}")
        if ai > clients:
            raise ProfileError(
                f"{ctx}: 'ai'={ai} exceeds clients={clients}; "
                f"server would refuse the extra clients"
            )

        teams.append(Team(name=name, ai=ai))

    return tuple(teams)
