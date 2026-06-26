import logging
import time
import os
import json
from datetime import datetime
from utils.config_loader import get_config


class JsonFormatter(logging.Formatter):
    """Format log records into structured JSON."""

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
            "inventory",
        ]:
            if hasattr(record, attr):
                log_obj[attr] = getattr(record, attr)
        return json.dumps(log_obj)


class AILogger:
    """Handles structured JSONL file logging and optional terminal output."""

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

    def configure(
        self,
        team_name: str,
        config_dict: dict | None = None,
        verbose: str | None = None,
    ):
        """Setup output files, directories, and terminal stream handlers."""
        run_id_env = os.environ.get("ZAPPY_RUN_ID")
        if not run_id_env:
            run_id_env = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
            os.environ["ZAPPY_RUN_ID"] = run_id_env
        self.run_id = str(run_id_env)

        log_dir = os.path.join(os.path.dirname(__file__), "..", "logs", self.run_id)
        self.log_dir = log_dir
        os.makedirs(log_dir, exist_ok=True)
        os.makedirs(os.path.join(log_dir, "metrics"), exist_ok=True)
        os.makedirs(os.path.join(log_dir, "ai"), exist_ok=True)
        os.makedirs(os.path.join(log_dir, "network"), exist_ok=True)

        if config_dict:
            config_path = os.path.join(log_dir, "config.json")
            if not os.path.exists(config_path):
                full_config = get_config().copy()
                full_config["client"] = config_dict
                with open(config_path, "w") as f:
                    json.dump(full_config, f, indent=4)

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

        if verbose in ["ai", "both"]:
            sh_ai = logging.StreamHandler()
            sh_ai.setLevel(logging.DEBUG)
            sh_ai.setFormatter(
                logging.Formatter("%(asctime)s - [AI] - %(message)s", "%H:%M:%S")
            )
            self.logger.addHandler(sh_ai)

        if verbose in ["network", "both"]:
            sh_net = logging.StreamHandler()
            sh_net.setLevel(logging.DEBUG)
            sh_net.setFormatter(
                logging.Formatter("%(asctime)s - [NET] - %(message)s", "%H:%M:%S")
            )
            self.net_logger.addHandler(sh_net)

    def get_log_dir(self) -> str | None:
        return getattr(self, "log_dir", None)

    def dump_metrics(self):
        """Save AI run statistics (PID, survival time, highest level) to JSON."""
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

    def warning(self, msg: str):
        self.logger.warning("warning", extra={"event": f"WARNING: {msg}"})

    def error(self, msg: str):
        self.logger.error("error", extra={"event": f"ERROR: {msg}"})

    def log_state(self, state: str, action: str, level: int | str, inventory):
        """Log the drone's current FSM state, intended action, and resources."""
        inv_str = f"Food:{inventory.food} Lm:{inventory.linemate} Der:{inventory.deraumere} Sib:{inventory.sibur} Men:{inventory.mendiane} Phi:{inventory.phiras} Thy:{inventory.thystame}"
        msg = f"Lvl:{level} | {inv_str} | State:{state} | Action:{action}"

        if isinstance(level, int):
            self.highest_level = max(self.highest_level, level)

        if msg != self.last_state_log:
            self.logger.info(
                msg,
                extra={
                    "state": state,
                    "action": action,
                    "level": level,
                    "inventory": inv_str,
                },
            )
            self.last_state_log = msg

    def log_send(self, message: str):
        self.last_command_time = time.perf_counter()
        self.last_command = message

    def log_receive(self, response: str):
        """Record incoming network payload and calculate round-trip latency."""
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
