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
    READY = "READY"
    COMING = "COMING"
    LEAVING = "LEAVING"
    INCANT = "INCANT"
    ABORT = "ABORT"


@dataclass
class DecodedBroadcast:
    """The structured content of a broadcast payload (the part we control)."""

    team_name: str
    msg_type: MessageType
    level: int
    drone_id: str = ""


class BroadcastProtocol:
    @staticmethod
    def parse_message(message_line: str) -> tuple[int, str]:
        """Parse 'message <direction>,<payload>' from server."""
        _, rest = message_line.split(" ", 1)
        direction_str, payload = rest.split(",", 1)
        return int(direction_str), payload.lstrip()

    @staticmethod
    def encode(
        team_name: str, msg_type: MessageType, level: int, drone_id: str = ""
    ) -> str:
        if drone_id:
            return f"{team_name}|{msg_type.value}|{level}|{drone_id}"
        return f"{team_name}|{msg_type.value}|{level}"

    @staticmethod
    def decode(raw_message: str) -> DecodedBroadcast:
        try:
            parts = raw_message.split("|")
            if len(parts) == 3:
                team_name, msg_type, level = parts
                drone_id = ""
            elif len(parts) == 4:
                team_name, msg_type, level, drone_id = parts
            else:
                raise ValueError("Invalid number of fields in broadcast message")
            msg_type_enum = MessageType(msg_type)
            return DecodedBroadcast(team_name, msg_type_enum, int(level), drone_id)
        except ValueError as e:
            raise ValueError("Invalid broadcast message format") from e
