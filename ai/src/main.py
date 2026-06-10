##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## main
##

from fsm import AIController
from context import DroneContext
from NetworkBuffer import NetworkBuffer
from tcpClient import TcpClient
from argsParser import parseArgs

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
    _fsm: AIController

    def __init__(self, config):
        print("[Orchestrator] Initializing Zappy AI client...")
        client = TcpClient(host=config.host, port=config.port)
        client.connect()
        slots, (w, h) = client.handshake(config.teamName)

        context = DroneContext(team_name=config.teamName)
        context.available_slots = slots
        context.map_width = w
        context.map_height = h

        net = NetworkBuffer(client)
        fsm = AIController(context)

        self._net = net
        self._fsm = fsm

    def run(self):
        pending_command = None
        while True:
            self._net.poll()

            while (event := self._net.next_event()) is not None:
                self._handle_event(self._fsm.context, event)

            if pending_command is None:
                command = self._fsm.tick()
                if command:
                    self._net.send_command(command)
                    pending_command = command

            if pending_command is not None:
                response = self._net.next_response()
                if response is not None:
                    self._handle_response(self._fsm.context, pending_command, response)
                    pending_command = None

    def _handle_event(self, context: DroneContext, event: str):
        # For now, we just print the raw event. In a full implementation,
        # this would parse the event and update the context accordingly.
        print(f"[Event] {event}")
    
    def _handle_response(self, context: DroneContext, command: str, response: str):
        # For now, we just print the raw response. In a full implementation,
        # this would parse the response and update the context accordingly.
        print(f"[Response] {response}")

def main():
    config = parseArgs()

    orchestrator = Orchestrator(config)
    orchestrator.run()

if __name__ == "__main__":
    main()