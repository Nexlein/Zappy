from __future__ import annotations

from dataclasses import dataclass

from .commands import Binaries, ai_command, gui_command, server_command
from .process import ManagedProcess
from .profiles import Profile
from .supervisor import Supervisor


@dataclass(frozen=True)
class Launch:
    """Handle on the processes a single profile booted: the server, its AIs
    and the optional GUI, plus the port they all share."""

    port: int
    server: ManagedProcess
    ais: tuple[ManagedProcess, ...]
    gui: ManagedProcess | None


def launch_profile(
    supervisor: Supervisor,
    profile: Profile,
    binaries: Binaries = Binaries(),
    host: str | None = None,
) -> Launch:
    """Boot a full game: server, then one AI per team slot, then the GUI if the
    profile asks for it. All share one freshly allocated port."""
    port = supervisor.ports.allocate()

    server = supervisor.spawn(
        "server", server_command(binaries.server, port, profile), port=port
    )

    ais = tuple(
        supervisor.spawn(
            f"{team.name}-{i}",
            ai_command(binaries.ai, port, team.name, host, team.strategy),
        )
        for team in profile.teams
        for i in range(team.ai)
    )

    gui = None
    if profile.auto_gui:
        gui = supervisor.spawn("gui", gui_command(binaries.gui, port, host))

    return Launch(port=port, server=server, ais=ais, gui=gui)
