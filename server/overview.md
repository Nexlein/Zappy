# Zappy Server

C game server (built in C++). Single process, single thread, `poll()`-driven.
Owns the world, enforces the rules, talks to AI clients and GUI clients over one TCP port.

> Deep dive: full protocol tables, per-class notes and design rationale live in
> [the implementation reference](doc.md). This page is the map.

## What it does

- Hosts the game world (toroidal tile map, resources, players, eggs, teams).
- Accepts two kinds of client on the same port; the handshake decides which:
  team name `GRAPHIC` → GUI, any other valid team → AI.
- Drives all timing off a scheduler; no busy-waiting, idle CPU near zero.

## Run

```
./zappy_server -p port -x width -y height -n team1 team2 ... -c clientsNb -f freq
```

`-c` = slots per team, `-f` = time-unit frequency. `GRAPHIC` is reserved and rejected as a team name.

## Shape of the code

```
poll() ── readable fds ──► CommandDispatcher ──► AI / GUI handler ──► World mutates
   ▲                                                                      │
   └──────── timeout = Scheduler::msUntilNext() ◄── Scheduler ◄───────────┘
                                                    (fires timed game events,
                                                     GuiNotifier pushes to GUIs)
```

| Layer | Lives in | Job |
|-------|----------|-----|
| `network/` | `Listener`, `Connection`, `ClientManager` | sockets, line buffering, the `poll()` set |
| `core/` | `Server`, `World`, `Scheduler`, `GameClock`, `Args` | game state + logic + timing, no network knowledge |
| `protocol/` | `AiParser`, `GuiParser`, `Serializer` | parse client lines / build wire strings |
| `game/` | `CommandDispatcher`, `GuiNotifier`, `commands/` | route lines to handlers, broadcast state to GUIs |

Two rules hold the design together:

- **`World` is pure** — it mutates state and knows nothing about sockets or timers,
  so it is unit-testable in isolation. The server layer observes the result.
- **`Scheduler` owns all timing** — every action (`duration = cost / freq`) is a timed
  callback. The scheduler's next-fire time is fed straight to `poll()` as the timeout.

## Key concepts

- **Time:** an action of cost `c` takes `c / freq` seconds. `sst` (GUI changes freq)
  rescales every pending timer. Game ticks are the integral of freq over time (`GameClock`),
  so they stay correct across speed changes.
- **Resources** respawn every `20 / freq` seconds: `floor(w * h * density)` per type.
- **Incantation** is the elevation ritual — prerequisites checked at both start and end of
  the `300 / freq` ritual; participants frozen meanwhile; cross-team allowed.
- **Fork** lays an egg → adds a team slot → a new AI connection hatches it.

## Tests

- Unit: `Scheduler`, `CommandDispatcher`, `Serializer`, parsers, `World`, `broadcastDirection`.
- E2E (`tests/e2e/`): fork the real binary, talk raw TCP, assert on the wire bytes.

```
cmake --build build --target zappy_server zappy_server_e2e_tests && ./zappy_server_e2e_tests
```
