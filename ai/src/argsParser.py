##
## EPITECH PROJECT, 2026
## Zappy
## File description:
## argsParser
##

from argparse import ArgumentParser
from dataclasses import dataclass
import sys
from utils.config_loader import get_client_config


@dataclass
class Config:
    port: int
    teamName: str
    host: str
    strategy: str
    verbose: str | None


def parseArgs() -> Config:
    if "--help" in sys.argv:
        print("USAGE: ./zappy_ai -p PORT -n NAME [-h HOST] [-s STRATEGY]")
        print("")
        print("option\t\tdescription")
        print("-p port\t\tport number")
        print("-n name\t\tname of the team")
        print("-h machine\tname of the machine; localhost by default")
        print("-s strategy\tAI strategy: fsm, utility, uai or queen; fsm by default")
        print("-v [type]\tVerbose terminal logging: network, ai, or both (default)")
        sys.exit(0)

    defaults = get_client_config()

    parser = ArgumentParser(description="Zappy AI Client", add_help=False)
    parser.add_argument(
        "-p",
        "--port",
        type=int,
        default=defaults.get("port", None),
        help="Port number to connect to the server",
    )
    parser.add_argument(
        "-n",
        "--name",
        type=str,
        default=defaults.get("teamName", None),
        help="Name of the team",
    )
    parser.add_argument(
        "-h",
        "--host",
        type=str,
        default=defaults.get("host", "localhost"),
        help="Hostname of the ai client (default: localhost)",
    )
    parser.add_argument(
        "-s",
        "--strategy",
        type=str,
        default=defaults.get("strategy", "fsm"),
        choices=["fsm", "utility", "uai", "queen"],
        help="AI strategy: fsm, utility, uai or queen (default: fsm)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        nargs="?",
        const="both",
        choices=["network", "ai", "both"],
        help="Enable verbose terminal logging",
    )

    args = parser.parse_args()

    if args.port is None or args.name is None:
        print(
            "Error: -p (port) and -n (name) must be provided either via CLI or config.json",
            file=sys.stderr,
        )
        sys.exit(84)

    return Config(
        port=args.port,
        teamName=args.name,
        host=args.host,
        strategy=args.strategy,
        verbose=args.verbose,
    )
