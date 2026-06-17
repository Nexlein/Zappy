# Zappy

Zappy is a multiplayer network game. Autonomous AI players form teams and compete on a flat, wrap-around map called Trantor. The first team to have six players reach the maximum elevation (level 8) wins the game.

We split the project into three distinct components that communicate over TCP sockets.

## Components

### 1. Server (`zappy_server`)

Written in C++. It manages the game state, handles time, spawns resources, and enforces rules. You run one server instance per game.

### 2. Graphic Client (`zappy_gui`)

Written in C++. It connects to the server as a spectator. It requests the map state and renders the world, the players, and the resources in real-time. It authenticates using the special team name `GRAPHIC`.

*Feature:* We added a `--headless` flag. Run the GUI in text-only mode to capture event logs without launching a graphics window.

### 3. AI Client (`zappy_ai`)

Written in Python. Each AI connects to the server as an individual player belonging to a team. The AI operates autonomously.

*Feature:* We implemented two different AI architectures. You can select the agent's brain at launch:

- **Finite State Machine (`-s fsm`)**: A strict, rule-based decision engine.
- **Utility AI (`-s utility`)**: A dynamic, score-based system that evaluates immediate needs (hunger, leveling, grouping) and picks the highest-scoring action every tick.

## Mechanics

Players burn 1 unit of food every 126 time units. If food hits zero, the player dies. The server spawns food and stones (linemate, deraumere, sibur, mendiane, phiras, thystame) across the map.

To level up, players perform an elevation ritual. They gather on a specific tile with the required stones and the exact number of same-level players. The ritual consumes the stones.

## Getting Started

Compile the project. Run the binaries in separate terminals.

**Start the server:**

```bash
./zappy_server -p 4242 -x 20 -y 20 -n TeamA TeamB -c 5 -f 100
```

This creates a 20x20 map on port 4242 with two teams. Each team can have 5 players. The execution frequency is 100.

**Start the GUI:**

```bash
./zappy_gui -p 4242 -h 127.0.0.1 [--headless]
```

**Spawn an AI player:**

```bash
./zappy_ai -p 4242 -n TeamA -h 127.0.0.1 -s utility -v
```

Use `-s fsm` or `-s utility` to choose the brain. Use `-v` to enable verbose terminal logging.
