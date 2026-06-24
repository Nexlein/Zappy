import sys
from pathlib import Path

from app import ZappyTUI
from supervisor.profiles import ProfileError, load_profiles

PROFILES = Path(__file__).resolve().parent.parent / "config" / "profiles.toml"


def main():
    try:
        profiles = load_profiles(PROFILES)
    except ProfileError as e:
        sys.exit(f"zappy_tui: {e}")
    ZappyTUI(profiles).run()


if __name__ == "__main__":
    main()
