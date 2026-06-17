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

logging.addLevelName(25, "AI_STATE")


class JsonFormatter(logging.Formatter):
    def format(self, record):
        log_obj = {
            "timestamp": datetime.fromtimestamp(record.created).isoformat(),
            "level": record.levelname,
            "pid": os.getpid(),
            "message": record.getMessage(),
        }
        for attr in [
            "command",
            "latency_ms",
            "state",
            "action",
            "response",
            "event",
            "level_num",
            "food",
        ]:
            if hasattr(record, attr):
                log_obj[attr] = getattr(record, attr)
        return json.dumps(log_obj)


class ColorFormatter(logging.Formatter):
    def __init__(self, team_name, logger_ref):
        super().__init__()
        self.team_name = team_name
        self.logger_ref = logger_ref

    def format(self, record):
        time_str = datetime.fromtimestamp(record.created).strftime("%H:%M:%S")
        drone_id = self.logger_ref.drone_id if self.logger_ref.drone_id else os.getpid()

        CYAN = "\033[96m"
        GREEN = "\033[92m"
        YELLOW = "\033[93m"
        MAGENTA = "\033[95m"
        RESET = "\033[0m"

        prefix = f"{CYAN}{time_str} | [{self.team_name}-{drone_id}]{RESET}"

        if record.levelno == 25:
            lvl = getattr(record, "level_num", "?")
            food = getattr(record, "food", "?")
            if isinstance(food, int):
                food = f"{food:02d}"
            state = getattr(record, "state", "Unknown")
            action = getattr(record, "action", "None")
            return f"{prefix} Lvl:{YELLOW}{lvl}{RESET} | Food:{YELLOW}{food}{RESET} | State:{GREEN}{state:12}{RESET} | Action:{MAGENTA}{action}{RESET}"

        return f"{prefix} {record.getMessage()}"


class AILogger:
    def __init__(self):
        self.logger = logging.getLogger("ZappyAI")
        self.logger.setLevel(logging.DEBUG)

        self.ch = logging.StreamHandler()
        self.ch.setLevel(25)
        self.ch.setFormatter(logging.Formatter("%(message)s"))

        if not self.logger.handlers:
            self.logger.addHandler(self.ch)

        self.last_command_time = 0.0
        self.last_command = ""
        self.last_state_log = ""
        self.drone_id: str | None = None

    def configure(self, team_name: str, verbose: bool):
        if verbose:
            self.ch.setLevel(logging.DEBUG)

        self.ch.setFormatter(ColorFormatter(team_name, self))

        log_dir = os.path.join(os.path.dirname(__file__), "..", "logs")
        os.makedirs(log_dir, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        pid = os.getpid()
        log_filename = f"ai_{team_name}_{timestamp}_{pid}.jsonl"

        fh = logging.FileHandler(os.path.join(log_dir, log_filename), mode="w")
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(JsonFormatter())

        self.logger.addHandler(fh)

    def info(self, msg: str):
        self.logger.info(msg)

    def warning(self, msg: str):
        self.logger.warning(msg)

    def error(self, msg: str):
        self.logger.error(msg)

    def talk(self, msg: str):
        self.logger.info(msg)

    def log_state(self, state: str, action: str, context=None):
        lvl = context.level if context else "?"
        food = context.inventory.food if context else "?"
        msg = f"Lvl:{lvl} | Food:{food} | State:{state} | Action:{action}"

        if msg != self.last_state_log:
            self.logger.log(
                25,
                msg,
                extra={
                    "state": state,
                    "action": action,
                    "level_num": lvl,
                    "food": food,
                },
            )
            self.last_state_log = msg

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

    def log_event(self, event: str, parsed_info: str = None):
        human_readable = parsed_info if parsed_info else event
        self.logger.warning(f"[EVENT] {human_readable}", extra={"event": event})


ai_logger = AILogger()
