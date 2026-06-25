from __future__ import annotations

from dataclasses import dataclass

from .commands import Binaries, ai_command, gui_command, server_command
from .process import ManagedProcess
from .profiles import Profile
from .supervisor import Supervisor


@dataclass
class Launch:
    """Handle on the processes a single profile booted: the server, its AIs and
    its GUIs, plus the port they all share. The ``ais`` and ``guis`` lists are
    mutable so the UI can attach more after launch (dead ones are kept so the
    list still maps to what the user sees)."""

    port: int
    server: ManagedProcess
    ais: list[ManagedProcess]
    guis: list[ManagedProcess]
    binaries: Binaries = Binaries()
    host: str | None = None


def launch_profile(
    supervisor: Supervisor,
    profile: Profile,
    binaries: Binaries = Binaries(),
    host: str | None = None,
) -> Launch:
    """Boot a full game: server, then one AI per team slot, then a GUI if the
    profile asks for it. All share one freshly allocated port. The returned
    handle remembers the binaries/host so later attaches reuse them."""
    port = supervisor.ports.allocate()

    server = supervisor.spawn(
        "server", server_command(binaries.server, port, profile), port=port
    )

    launch = Launch(
        port=port, server=server, ais=[], guis=[], binaries=binaries, host=host
    )

    for team in profile.teams:
        for _ in range(team.ai):
            attach_ai(supervisor, launch, team.name, team.strategy)

    if profile.auto_gui:
        attach_gui(supervisor, launch)

    return launch


def attach_ai(
    supervisor: Supervisor,
    launch: Launch,
    team: str,
    strategy: str | None = None,
) -> ManagedProcess:
    """Spawn one more AI for ``team`` on the running game's port and track it on
    the launch. Named ``<team>-<n>`` where n is the count of AIs already raised
    for that team (dead included, so names never collide)."""
    n = sum(1 for ai in launch.ais if ai.name.rsplit("-", 1)[0] == team)
    ai = supervisor.spawn(
        f"{team}-{n}",
        ai_command(launch.binaries.ai, launch.port, team, launch.host, strategy),
    )
    launch.ais.append(ai)
    return ai


def attach_gui(supervisor: Supervisor, launch: Launch) -> ManagedProcess:
    """Spawn one more GUI on the running game's port and track it. GUIs stack;
    each gets a distinct ``gui-<n>`` name."""
    gui = supervisor.spawn(
        f"gui-{len(launch.guis)}",
        gui_command(launch.binaries.gui, launch.port, launch.host),
    )
    launch.guis.append(gui)
    return gui
