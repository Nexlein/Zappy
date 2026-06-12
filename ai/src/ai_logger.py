##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## AI Logger
##

import logging
import time
import os
import json
from datetime import datetime

logging.addLevelName(25, "AI_TALK")


class JsonFormatter(logging.Formatter):
    def format(self, record):
        log_obj = {
            "timestamp": datetime.fromtimestamp(record.created).isoformat(),
            "level": record.levelname,
            "pid": os.getpid(),
            "message": record.getMessage(),
        }
        if hasattr(record, "command"):
            log_obj["command"] = getattr(record, "command")
        if hasattr(record, "latency_ms"):
            log_obj["latency_ms"] = getattr(record, "latency_ms")
        if hasattr(record, "state"):
            log_obj["state"] = getattr(record, "state")
        if hasattr(record, "action"):
            log_obj["action"] = getattr(record, "action")
        if hasattr(record, "response"):
            log_obj["response"] = getattr(record, "response")
        if hasattr(record, "event"):
            log_obj["event"] = getattr(record, "event")
        return json.dumps(log_obj)


class NoTalkFilter(logging.Filter):
    def filter(self, record):
        return record.levelno != 25


class AILogger:
    def __init__(self):
        self.logger = logging.getLogger("ZappyAI")
        self.logger.setLevel(logging.DEBUG)

        # Terminal handler (StreamHandler)
        self.ch = logging.StreamHandler()
        # Default to showing AI_TALK and above
        self.ch.setLevel(25)

        formatter = logging.Formatter(
            "%(asctime)s | %(levelname)-7s | %(message)s", datefmt="%H:%M:%S"
        )
        self.ch.setFormatter(formatter)

        if not self.logger.handlers:
            self.logger.addHandler(self.ch)

        self.last_command_time = 0.0
        self.last_command = ""

    def configure(self, team_name: str, verbose: bool):
        if verbose:
            self.ch.setLevel(logging.DEBUG)

        log_dir = os.path.join(os.path.dirname(__file__), "..", "logs")
        os.makedirs(log_dir, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        pid = os.getpid()
        log_filename = f"ai_{team_name}_{timestamp}_{pid}.jsonl"

        fh = logging.FileHandler(os.path.join(log_dir, log_filename), mode="w")
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(JsonFormatter())
        fh.addFilter(NoTalkFilter())  # Filter out AI_TALK from JSON file

        self.logger.addHandler(fh)

    def talk(self, msg: str):
        self.logger.log(25, msg)

    def log_state(self, state: str, action: str):
        self.logger.info(
            f"[{state}] {action}", extra={"state": state, "action": action}
        )

    def log_send(self, message: str):
        self.last_command_time = time.perf_counter()
        self.last_command = message
        self.logger.info(
            f"[NETWORK OUT] Sent: {repr(message)}", extra={"command": message}
        )

    def log_receive(self, response: str):
        if self.last_command_time > 0:
            elapsed = time.perf_counter() - self.last_command_time
            ms = int(elapsed * 1000)
            self.logger.info(
                f"[NETWORK IN] Recv (to {repr(self.last_command)} in {ms}ms): {repr(response)}",
                extra={
                    "command": self.last_command,
                    "latency_ms": ms,
                    "response": response,
                },
            )
            self.last_command_time = 0.0
            self.last_command = ""
        else:
            self.logger.info(
                f"[NETWORK IN] Recv: {repr(response)}", extra={"response": response}
            )

    def log_event(self, event: str):
        self.logger.warning(f"[EVENT] {event}", extra={"event": event})

    def info(self, msg: str):
        self.logger.info(msg)

    def error(self, msg: str):
        self.logger.error(msg)


ai_logger = AILogger()
