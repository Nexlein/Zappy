from textual.widgets import OptionList
from textual.widgets.option_list import Option

from supervisor.profiles import Profile


class ProfileList(OptionList):
    def __init__(self, profiles: dict[str, Profile]) -> None:
        options = [Option(name, id=name) for name in profiles]
        super().__init__(*options)

    def on_mount(self) -> None:
        self.border_title = "Profiles"
