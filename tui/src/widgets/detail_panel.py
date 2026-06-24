from textual.widgets import Static

from supervisor.process import ManagedProcess
from supervisor.profiles import Profile


class DetailPanel(Static):
    """Right pane: details of whatever is highlighted — a profile's config or a
    running process's state. The app feeds it on selection changes."""

    def on_mount(self) -> None:
        self.border_title = "Details"

    def show_profile(self, profile: Profile) -> None:
        teams = "\n".join(
            f"  - {t.name}: {t.ai} ai ({t.strategy or 'default'})"
            for t in profile.teams
        )
        self.update(
            f"profile: {profile.name}\n"
            f"map:     {profile.width}x{profile.height}\n"
            f"clients: {profile.clients}\n"
            f"freq:    {profile.freq}\n"
            f"auto_gui: {profile.auto_gui}\n"
            f"teams:\n{teams}"
        )

    def show_process(self, process: ManagedProcess) -> None:
        state = "alive" if process.is_alive() else "dead"
        self.update(
            f"process: {process.name}\n"
            f"pid:     {process.pid}\n"
            f"state:   {state}\n"
            f"rc:      {process.returncode}\n"
            f"command:\n  {' '.join(process.command)}"
        )
