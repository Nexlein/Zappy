##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## BroadcastProtocol
##

from dataclasses import dataclass
from enum import Enum


class MessageType(Enum):
    RALLY = "RALLY"
    RALLY_FULL = "RALLY_FULL"
    READY = "READY"
    COMING = "COMING"
    LEAVING = "LEAVING"
    INCANT = "INCANT"
    ABORT = "ABORT"
    SWARM_INVENTORY = "SWARM_INVENTORY"


@dataclass
class DecodedBroadcast:
    """The structured content of a broadcast payload (the part we control)."""

    team_name: str
    msg_type: MessageType
    level: int
    drone_id: str = ""
    tail: str = ""


class BroadcastProtocol:
    @staticmethod
    def parse_message(message_line: str) -> tuple[int, str]:
        """Parse 'message <direction>,<payload>' from server."""
        _, rest = message_line.split(" ", 1)
        direction_str, payload = rest.split(",", 1)
        return int(direction_str), payload.lstrip()

    @staticmethod
    def encode(
        team_name: str,
        msg_type: MessageType,
        level: int,
        drone_id: str = "",
        tail: str = "",
    ) -> str:
        base = f"{team_name}|{msg_type.value}|{level}"
        if drone_id:
            base += f"|{drone_id}"
        if tail:
            base += f"|{tail}"
        return base

    @staticmethod
    def decode(raw_message: str) -> DecodedBroadcast:
        try:
            parts = raw_message.split("|")
            if len(parts) < 3:
                raise ValueError("Invalid number of fields in broadcast message")

            team_name = parts[0]
            msg_type = parts[1]
            level = parts[2]
            drone_id = parts[3] if len(parts) > 3 else ""
            tail = "|".join(parts[4:]) if len(parts) > 4 else ""

            msg_type_enum = MessageType(msg_type)
            return DecodedBroadcast(
                team_name, msg_type_enum, int(level), drone_id, tail
            )
        except ValueError as e:
            raise ValueError("Invalid broadcast message format") from e
