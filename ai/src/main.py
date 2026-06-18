##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## main
##

from ai_factory import create_ai_controller
from context import DroneContext, BroadcastMessage
from NetworkBuffer import NetworkBuffer
from tcpClient import TcpClient
from argsParser import parseArgs, Config
from BroadcastProtocol import BroadcastProtocol, MessageType
from look_parser import parse_look_to_tiles
from inventory_parser import update_inventory
from typing import Any
from ai_logger import ai_logger
import subprocess
import sys
import signal
import time
from utils.config_loader import get_network_config


class DroneDied(Exception):
    """Raised when the server announces this drone's death."""


class Orchestrator:
    """
    The main orchestrator for the Zappy AI client.

    Responsibilities:
      - Parse command-line arguments
      - Establish TCP connection and perform handshake
      - Initialize shared context and FSM controller
      - Main loop: poll network, update context, tick FSM, send commands
    """

    _net: NetworkBuffer
    _controller: Any
    _context: DroneContext
    _config: Config

    def __init__(self, config: Config):
        client = None
        while True:
            try:
                client = TcpClient(host=config.host, port=config.port)
                client.connect()
                slots, (w, h) = client.handshake(config.teamName)
                break
            except ConnectionRefusedError as e:
                if "Team is full" in str(e):
                    ai_logger.error(f"[Orchestrator] {e}. Exiting.")
                    sys.exit(0)
                ai_logger.warning(
                    "[Orchestrator] Connection refused. Retrying in 1s..."
                )
                if client and client._socket:
                    client._socket.close()
                time.sleep(1)
            except (ConnectionError, ValueError) as e:
                ai_logger.warning(
                    f"[Orchestrator] Handshake failed ({e}). Retrying in 1s..."
                )
                if client and client._socket:
                    client._socket.close()
                time.sleep(1)

        if not client:
            raise RuntimeError("Failed to initialize TcpClient")

        self._context = DroneContext(team_name=config.teamName)
        ai_logger.drone_id = self._context.drone_id[:4]
        self._context.available_slots = slots
        self._context.map_width = w
        self._context.map_height = h
        self._config = config

        self._net = NetworkBuffer(client)
        self._controller = create_ai_controller(config.strategy, self._context)
        self._pending_command: str | None = None

    def run(self):
        sleep_sec = get_network_config().get("POLL_SLEEP_SEC", 0.005)
        try:
            while True:
                self._net.poll()

                while (event := self._net.next_event()) is not None:
                    self._handle_event(event)

                if (
                    self._pending_command is None
                    and not self._context.elevation_in_progress
                ):
                    command = self._controller.tick()
                    self._context.broadcasts.clear()
                    if command:
                        self._net.send_command(command)
                        self._pending_command = command

                response = self._net.next_response()
                if response is not None:
                    self._handle_response(self._pending_command, response)

                time.sleep(sleep_sec)
        except (DroneDied, ConnectionError):
            pass

    def _handle_event(self, event: str):
        if event.startswith("message"):
            try:
                direction, payload = BroadcastProtocol.parse_message(event)
                decoded = BroadcastProtocol.decode(payload)
            except ValueError:
                return

            if decoded.team_name != self._context.team_name:
                return

            if decoded.level != self._context.level:
                # We still append it below because some FSM might care, but usually they ignore wrong levels.
                pass

            if (
                self._context.elevation_in_progress
                and decoded.msg_type == MessageType.ABORT
                and decoded.level == self._context.level
            ):
                self._context.elevation_in_progress = False

            self._context.broadcasts.append(BroadcastMessage(direction, decoded))
        elif event.startswith("eject"):
            self._context.vision.clear()
        elif event.startswith("dead"):
            raise DroneDied()
        elif event.startswith("Elevation underway"):
            self._context.elevation_in_progress = True
        elif event.startswith("Current level:"):
            try:
                level = int(event.split(":")[1].strip())
            except ValueError:
                ai_logger.error(
                    f"[Orchestrator] Could not parse level from event: {event}"
                )
                return
            self._context.level = level
            self._context.elevation_in_progress = False
            self._context.vision.clear()
            if self._pending_command == "Incantation":
                # Our own ritual's success reply, routed here as an event.
                self._context.last_command_successful = True
                self._pending_command = None

    def _handle_response(self, command: str | None, response: str):
        ai_logger.log_receive(response)

        if (
            self._context.elevation_in_progress
            and response == "ko"
            and command != "Incantation"
        ):
            self._context.elevation_in_progress = False
            return

        if command is None:
            ai_logger.error(
                f"[Orchestrator] Response with no pending command: {response}"
            )
            return

        handlers = {
            "Incantation": self._handle_incantation_response,
            "Look": self._handle_look_response,
            "Inventory": self._handle_inventory_response,
            "Connect_nbr": self._handle_connect_nbr_response,
            "Fork": self._handle_fork_response,
        }

        # Check exact matches
        if command in handlers:
            handlers[command](response)
        # Check prefix matches
        elif command.startswith("Take "):
            self._handle_take_response(command, response)
        elif command.startswith("Set "):
            self._handle_set_response(command, response)
        elif command in ("Forward", "Right", "Left"):
            if response == "ok":
                self._context.vision.clear()

        self._context.last_command_successful = response != "ko"
        self._pending_command = None

    def _handle_incantation_response(self, response: str):
        self._context.elevation_in_progress = False
        self._context.last_command_successful = response != "ko"

    def _handle_look_response(self, response: str):
        try:
            self._context.vision = parse_look_to_tiles(response)
        except ValueError as e:
            ai_logger.error(f"[Orchestrator] Look parse error: {e}")

    def _handle_inventory_response(self, response: str):
        try:
            update_inventory(self._context.inventory, response)
        except ValueError as e:
            ai_logger.error(f"[Orchestrator] Inventory parse error: {e}")
        self._context.ticks_since_inventory = 0

    def _handle_take_response(self, command: str, response: str):
        if response == "ok":
            resource = command.removeprefix("Take ").strip()
            if self._context.vision:
                tile = self._context.vision[0]
                setattr(tile, resource, max(0, getattr(tile, resource, 0) - 1))
            inv = self._context.inventory
            setattr(inv, resource, getattr(inv, resource, 0) + 1)
        elif response == "ko":
            self._context.vision.clear()

    def _handle_set_response(self, command: str, response: str):
        if response == "ok":
            resource = command.removeprefix("Set ").strip()
            inv = self._context.inventory
            setattr(inv, resource, max(0, getattr(inv, resource, 0) - 1))
            if self._context.vision:
                tile = self._context.vision[0]
                setattr(tile, resource, getattr(tile, resource, 0) + 1)
        elif response == "ko":
            self._context.vision.clear()

    def _handle_connect_nbr_response(self, response: str):
        if response.isdigit():
            self._context.available_slots = int(response)

    def _handle_fork_response(self, response: str):
        if response == "ok":
            subprocess.Popen(
                [
                    sys.executable,
                    sys.argv[0],
                    "-p",
                    str(self._config.port),
                    "-n",
                    self._config.teamName,
                    "-h",
                    self._config.host,
                    "-s",
                    self._config.strategy,
                ]
            )


def main():
    config = parseArgs()

    config_dict = {
        "port": config.port,
        "teamName": config.teamName,
        "host": config.host,
        "strategy": config.strategy,
    }
    ai_logger.configure(config.teamName, config_dict)

    signal.signal(signal.SIGCHLD, signal.SIG_IGN)

    orchestrator = Orchestrator(config)

    try:
        orchestrator.run()
    finally:
        ai_logger.dump_metrics()

        # Auto generate charts
        log_dir = ai_logger.get_log_dir()
        if log_dir:
            import os

            report_script = os.path.join(
                os.path.dirname(__file__), "generate_charts.py"
            )
            subprocess.run(
                [sys.executable, report_script, log_dir],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(
            "\n[Zappy AI] Shutting down due to KeyboardInterrupt.",
            file=sys.stderr,
        )
        sys.exit(0)
