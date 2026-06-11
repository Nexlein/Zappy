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
from argsParser import parseArgs
from BroadcastProtocol import BroadcastProtocol
from look_parser import parse_look_to_tiles
from inventory_parser import update_inventory
from typing import Any

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

    def __init__(self, config):
        print("[Orchestrator] Initializing Zappy AI client...")
        client = TcpClient(host=config.host, port=config.port)
        client.connect()
        slots, (w, h) = client.handshake(config.teamName)

        self._context = DroneContext(team_name=config.teamName)
        self._context.available_slots = slots
        self._context.map_width = w
        self._context.map_height = h

        self._net = NetworkBuffer(client)
        self._controller = create_ai_controller(config.strategy, self._context)
        self._pending_command: str | None = None

    def run(self):
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
        except DroneDied:
            print("[Orchestrator] This drone has died. Exiting.")

    def _handle_event(self, event: str):
        if event.startswith("message"):
            try:
                direction, payload = BroadcastProtocol.parse_message(event)
                decoded = BroadcastProtocol.decode(payload)
            except ValueError:
                return
            if decoded.team_name != self._context.team_name:
                return
            self._context.broadcasts.append(BroadcastMessage(direction, decoded))
        elif event.startswith("eject"):
            self._context.vision.clear()
        elif event.startswith("dead"):
            raise DroneDied()
        elif event.startswith("Elevation underway"):
            print("[Orchestrator] Ritual started: drone is frozen until verdict.")
            self._context.elevation_in_progress = True
        elif event.startswith("Current level:"):
            try:
                level = int(event.split(":")[1].strip())
            except ValueError:
                print(f"[Orchestrator] Could not parse level from event: {event}")
                return
            self._context.level = level
            self._context.elevation_in_progress = False
            if self._pending_command == "Incantation":
                # Our own ritual's success reply, routed here as an event.
                self._context.last_command_successful = True
                self._pending_command = None

    def _handle_response(self, command: str | None, response: str):
        if (
            self._context.elevation_in_progress
            and response == "ko"
            and command != "Incantation"
        ):
            self._context.elevation_in_progress = False
            return
        if command is None:
            print(f"[Orchestrator] Response with no pending command: {response}")
            return
        if command == "Incantation":
            # Failure verdict of our own ritual (success arrives as an event).
            self._context.elevation_in_progress = False
            self._context.last_command_successful = response != "ko"
        elif command == "Look":
            self._context.vision = parse_look_to_tiles(response)
        elif command == "Inventory":
            update_inventory(self._context.inventory, response)
            self._context.ticks_since_inventory = 0
        elif command.startswith("Take"):
            if response == "ok":
                resource = command.removeprefix("Take ").strip()
                if self._context.vision:
                    tile = self._context.vision[0]
                    setattr(tile, resource, max(0, getattr(tile, resource, 0) - 1))
                inv = self._context.inventory
                setattr(inv, resource, getattr(inv, resource, 0) + 1)
        elif command.startswith("Set"):
            if response == "ok":
                resource = command.removeprefix("Set ").strip()
                inv = self._context.inventory
                setattr(inv, resource, max(0, getattr(inv, resource, 0) - 1))
                if self._context.vision:
                    tile = self._context.vision[0]
                    setattr(tile, resource, getattr(tile, resource, 0) + 1)
        elif command in ("Forward", "Right", "Left"):
            if response == "ok":
                self._context.vision.clear()
        self._context.last_command_successful = response != "ko"
        self._pending_command = None

def main():
    config = parseArgs()

    orchestrator = Orchestrator(config)
    orchestrator.run()

if __name__ == "__main__":
    main()