from __future__ import annotations

from dataclasses import dataclass

from .profiles import Profile


@dataclass(frozen=True)
class Binaries:
    """Paths to the three zappy executables. Defaults match the wrapper layout
    (binaries sit next to ``zappy_tui`` at the repo root)."""

    server: str = "./zappy_server"
    ai: str = "./zappy_ai"
    gui: str = "./zappy_gui"


def server_command(binary: str, port: int, profile: Profile) -> list[str]:
    """``-p -x -y -n <team names...> -c <clients> -f <freq>``."""
    args = [
        binary,
        "-p", str(port),
        "-x", str(profile.width),
        "-y", str(profile.height),
        "-n", *[team.name for team in profile.teams],
        "-c", str(profile.clients),
        "-f", str(profile.freq),
    ]
    return args


def ai_command(binary: str, port: int, team: str, host: str | None = None) -> list[str]:
    """``-p -n <team> [-h <host>]``."""
    args = [binary, "-p", str(port), "-n", team]
    if host is not None:
        args += ["-h", host]
    return args


def gui_command(binary: str, port: int, host: str | None = None) -> list[str]:
    """``-p [-h <host>]``."""
    args = [binary, "-p", str(port)]
    if host is not None:
        args += ["-h", host]
    return args
