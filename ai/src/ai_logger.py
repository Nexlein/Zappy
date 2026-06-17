import logging
import time
import os
import json
from datetime import datetime


class JsonFormatter(logging.Formatter):
    def format(self, record):
        log_obj = {
            "timestamp": datetime.fromtimestamp(record.created).isoformat(),
            "pid": os.getpid(),
        }
        for attr in [
            "command",
            "latency_ms",
            "state",
            "action",
            "response",
            "event",
            "level",
            "food",
        ]:
            if hasattr(record, attr):
                log_obj[attr] = getattr(record, attr)
        return json.dumps(log_obj)


class AILogger:
    def __init__(self):
        self.logger = logging.getLogger("ZappyAI")
        self.net_logger = logging.getLogger("ZappyAINet")
        self.logger.setLevel(logging.DEBUG)
        self.net_logger.setLevel(logging.DEBUG)
        self.logger.propagate = False
        self.net_logger.propagate = False

        self.last_command_time = 0.0
        self.last_command = ""
        self.last_state_log = ""
        self.drone_id: str | None = None

        self.start_time = time.time()
        self.highest_level = 1
        self.run_id = ""

    def configure(self, team_name: str, config_dict: dict | None = None):

        self.run_id = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        log_dir = os.path.join(os.path.dirname(__file__), "..", "logs", self.run_id)
        self.log_dir = log_dir
        os.makedirs(log_dir, exist_ok=True)
        os.makedirs(os.path.join(log_dir, "metrics"), exist_ok=True)
        os.makedirs(os.path.join(log_dir, "ai"), exist_ok=True)
        os.makedirs(os.path.join(log_dir, "network"), exist_ok=True)

        if config_dict:
            config_path = os.path.join(log_dir, "config.json")
            if not os.path.exists(config_path):
                with open(config_path, "w") as f:
                    json.dump(config_dict, f, indent=4)

        pid = os.getpid()
        fh_ai = logging.FileHandler(
            os.path.join(log_dir, "ai", f"{team_name}_{pid}.jsonl"), mode="w"
        )
        fh_ai.setLevel(logging.DEBUG)
        fh_ai.setFormatter(JsonFormatter())
        self.logger.addHandler(fh_ai)

        fh_net = logging.FileHandler(
            os.path.join(log_dir, "network", f"{team_name}_{pid}.jsonl"), mode="w"
        )
        fh_net.setLevel(logging.DEBUG)
        fh_net.setFormatter(JsonFormatter())
        self.net_logger.addHandler(fh_net)

    def get_log_dir(self) -> str | None:
        return getattr(self, "log_dir", None)

    def dump_metrics(self):
        log_dir = os.path.join(
            os.path.dirname(__file__), "..", "logs", self.run_id, "metrics"
        )
        os.makedirs(log_dir, exist_ok=True)

        survival_time = time.time() - self.start_time
        metrics = {
            "pid": os.getpid(),
            "parent_pid": os.getppid(),
            "drone_id": self.drone_id,
            "survival_time_sec": survival_time,
            "highest_level": self.highest_level,
        }

        metrics_file = os.path.join(log_dir, f"metrics_{os.getpid()}.json")
        with open(metrics_file, "w") as f:
            json.dump(metrics, f, indent=4)

    def log_state(self, state: str, action: str, level: int | str, food: int | str):
        msg = f"Lvl:{level} | Food:{food} | State:{state} | Action:{action}"

        if isinstance(level, int):
            self.highest_level = max(self.highest_level, level)

        if msg != self.last_state_log:
            self.logger.info(
                msg,
                extra={
                    "state": state,
                    "action": action,
                    "level": level,
                    "food": food,
                },
            )
            self.last_state_log = msg

    def log_send(self, message: str):
        self.last_command_time = time.perf_counter()
        self.last_command = message
        self.net_logger.info(
            f"[NETWORK OUT] Sent: {repr(message)}", extra={"command": message}
        )

    def log_receive(self, response: str):
        if self.last_command_time > 0:
            elapsed = time.perf_counter() - self.last_command_time
            ms = int(elapsed * 1000)
            self.net_logger.info(
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
            self.net_logger.info(
                f"[NETWORK IN] Recv: {repr(response)}", extra={"response": response}
            )


ai_logger = AILogger()
