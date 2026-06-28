# Zappy

Autonomous AI teams race to evolve on a flat, wrap-around tile world (Trantor).

The first team to push six players to the top elevation (level 8) wins.

---
![C++](https://img.shields.io/badge/C%2B%2B-20-blue)
![Python](https://img.shields.io/badge/Python-3-blue)

![Server CI](https://github.com/Nexlein/Zappy/actions/workflows/server.yml/badge.svg)
![GUI CI](https://github.com/Nexlein/Zappy/actions/workflows/gui.yml/badge.svg)
![AI CI](https://github.com/Nexlein/Zappy/actions/workflows/ai.yml/badge.svg)

![Last commit](https://img.shields.io/github/last-commit/Nexlein/Zappy)
![Contributors](https://img.shields.io/github/contributors/Nexlein/Zappy)
![Code size](https://img.shields.io/github/languages/code-size/Nexlein/Zappy)
![Top language](https://img.shields.io/github/languages/top/Nexlein/Zappy)

[![Contributors](https://contrib.rocks/image?repo=Nexlein/Zappy)](https://github.com/Nexlein/Zappy/graphs/contributors)

**📖 API docs**: [Server](https://nexlein.github.io/Zappy/server/) · [GUI](https://nexlein.github.io/Zappy/gui/)

---

## Components

The project is split into three main components and one bonus tool, all communicating over TCP.

### 1. Server (zappy_server)

**C++** • Manages game state, timing, and rules. Uses a single-threaded `poll()` loop to accept both GUI and AI connections.

### 2. Graphic Client (zappy_gui)

**C++ / Raylib** • A 3D spectator client that requests the map state and renders the world.

- *Feature:* Run with `--headless` for text-only logs.

### 3. AI Client (zappy_ai)

**Python** • An autonomous player that survives, gathers resources, and incants to level up.

- *Strategies (`-s`):* `fsm` (rule-based), `uai` (score-based) or `queen`.

### Bonus: TUI Orchestrator (zappy_tui)

**Python / Textual** • A terminal dashboard to monitor games, manage profiles, attach clients, and track live logs.

## Quick Start

Compile the project and run the binaries in separate terminals.

**1. Start the Server:**

```bash
./zappy_server -p 4242 -x 20 -y 20 -n TeamA TeamB -c 5 -f 100
```

**2. Start the GUI:**

```bash
./zappy_gui -p 4242 -h 127.0.0.1
```

**3. Spawn an AI:**

```bash
./zappy_ai -p 4242 -n TeamA -h 127.0.0.1 -s utility
```

**Bonus - Orchestrator:**

```bash
./zappy_tui
```

## Documentation

For deep dives, check the component directories:

- [Server Docs](server/overview.md) | [GUI Docs](gui/overview.md) | [AI Docs](ai/README.md)
