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
    INCANT = "INCANT"
    ABORT = "ABORT"


@dataclass
class DecodedBroadcast:
    """The structured content of a broadcast payload (the part we control)."""

    team_name: str
    msg_type: MessageType
    level: int


class BroadcastProtocol:
    @staticmethod
    def encode(team_name: str, msg_type: MessageType, level: int) -> str:
        return f"{team_name}|{msg_type.value}|{level}"

    @staticmethod
    def decode(raw_message: str) -> DecodedBroadcast:
        try:
            team_name, msg_type, level = raw_message.split("|")
            msg_type_enum = MessageType(msg_type)
            return DecodedBroadcast(team_name, msg_type_enum, int(level))
        except ValueError as e:
            raise ValueError("Invalid broadcast message format") from e
