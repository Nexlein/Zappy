from textual import events
from textual.widgets import OptionList
from textual.widgets.option_list import Option

from supervisor.profiles import Profile


class ProfileList(OptionList):
    def __init__(self, profiles: dict[str, Profile]) -> None:
        options = [Option(name, id=name) for name in profiles]
        super().__init__(*options)

    def on_mount(self) -> None:
        self.border_title = "Profiles"

    async def _on_click(self, event: events.Click) -> None:
        event.prevent_default()
        clicked = event.style.meta.get("option")
        if clicked is None or self._options[clicked].disabled:
            return
        self.highlighted = clicked
        if event.chain >= 2:
            self.action_select()
