import signal
import sys
from pathlib import Path

from app import ZappyTUI
from supervisor.profiles import ProfileError, load_profiles
from supervisor.signals import raise_on_signals
from supervisor.supervisor import Supervisor

PROFILES = Path(__file__).resolve().parent.parent / "config" / "profiles.toml"


def main():
    try:
        profiles = load_profiles(PROFILES)
    except ProfileError as e:
        sys.exit(f"zappy_tui: {e}")

    # the app owns the supervisor; the guard + context manager guarantee every
    # spawned child is killed on quit, Ctrl-C, or SIGTERM.
    with raise_on_signals(signal.SIGTERM), Supervisor() as sup:
        ZappyTUI(profiles, sup).run()


if __name__ == "__main__":
    main()
