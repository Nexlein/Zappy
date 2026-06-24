from textual.widgets import Static


class ProcessList(Static):
    """Left-bottom pane: the running children. Placeholder for now."""

    def on_mount(self) -> None:
        self.border_title = "Processes"
        self.update("(running processes here)")
