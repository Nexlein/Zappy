from textual.widgets import Static


class DetailPanel(Static):
    """Right pane: details of the current selection. Placeholder for now."""

    def on_mount(self) -> None:
        self.border_title = "Details"
        self.update("(selection details here)")
