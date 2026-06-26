# Zappy AI

Client AI for Zappy. Connects to server, controls drone, survives, evolves.

## Run

`./zappy_ai [-p PORT] [-n NAME] [-h HOST] [-s STRATEGY] [-v LOG_TYPE]`

Flags:

- `-p`: Server port
- `-n`: Team name
- `-h`: Host (default: localhost)
- `-s`: Strategy (`fsm`, `utility`, `uai`, `queen`. Default: `fsm`)
- `-v`: Terminal log type (`network`, `ai`, `both`)

All flags are optional. If not provided, the program will use default values. (In the config file)

## Architecture

- `src/fsm/`: Finite state machine logic.
- `src/network/`: TCP connection buffer and parsing.
- `src/protocol/`: Zappy server protocol logic.
- `src/uai/` & `src/queen/`: Alternative experimental strategies.
