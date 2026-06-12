##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## argsParser
##

from argparse import ArgumentParser
from dataclasses import dataclass
import sys


@dataclass
class Config:
    port: int
    teamName: str
    host: str
    strategy: str
    verbose: bool = False


def parseArgs() -> Config:
    if "--help" in sys.argv:
        print("USAGE: ./zappy_ai -p PORT -n NAME [-h HOST] [-s STRATEGY]")
        print("")
        print("option\t\tdescription")
        print("-p port\t\tport number")
        print("-n name\t\tname of the team")
        print("-h machine\tname of the machine; localhost by default")
        print("-s strategy\tAI strategy: fsm or utility; fsm by default")
        print("-v, --verbose\tEnable verbose output")
        sys.exit(0)

    parser = ArgumentParser(description="Zappy AI Client", add_help=False)
    parser.add_argument(
        "-p",
        "--port",
        type=int,
        required=True,
        help="Port number to connect to the server",
    )
    parser.add_argument(
        "-n", "--name", type=str, required=True, help="Name of the team"
    )
    parser.add_argument(
        "-h",
        "--host",
        type=str,
        default="localhost",
        help="Hostname of the ai client (default: localhost)",
    )
    parser.add_argument(
        "-s",
        "--strategy",
        type=str,
        default="fsm",
        choices=["fsm", "utility"],
        help="AI strategy: fsm or utility (default: fsm)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable verbose output",
    )

    args = parser.parse_args()
    return Config(
        port=args.port,
        teamName=args.name,
        host=args.host,
        strategy=args.strategy,
        verbose=args.verbose,
    )
