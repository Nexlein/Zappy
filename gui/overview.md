# Zappy GUI

C++ graphic client. Connects to the server as `GRAPHIC`, receives the world snapshot
then a stream of events, and renders the game in 3D with raylib.

> Deep dive: full protocol notes, every behavior and helper class, and design rationale
> live in [the implementation reference](doc.md). This page is the map.

## What it does

- One read-only viewer of the game; it never plays, only observes and draws.
- Polls the server each frame (`mct`, `stu`), applies incoming events to local state,
  renders when something changed.
- Reconnects on drop (exponential backoff) and restores the window where it closed.

## Run

```
./zappy_gui -p port [-h machine] [--headless] [--dev true|false] [--language english|french]
```

`--headless` swaps the raylib renderer for a text dump (testing). `--dev` / `F3` toggles the dev HUD.

## Shape of the code

```
TcpSocket → ProtocolParser → EventQueue → GameState → Renderer
                                              │
                                          WorldState (server-authoritative)
                                              +
                                          behaviors (visual-only, ticked per frame)
```

| Layer | Lives in | Job |
|-------|----------|-----|
| `network/` | `TcpSocket`, `ProtocolParser` | line-buffered socket, parse lines → `Event` variants |
| `core/` | `EventQueue`, `GameState`, `WorldState`, `App`, `Args`, `behaviors/` | hold state, apply events, animate |
| `renderer/` | `IRenderer`, `RaylibRenderer`, `HeadlessRenderer`, `raylib_helpers/` | draw the world / dump it |

Design spine:

- **Logical vs visual state are split.** `WorldState` is exactly what the server said
  (positions, levels, inventories). Smooth motion, particles and effects are **behaviors**
  layered on top — `MoveBehavior`, `LevelUpBehavior`, `DeathBehavior`, etc. — and the
  headless renderer ignores all of it.
- **Dirty flag.** The handshake floods 100+ events, then updates are sparse. `GameState`
  marks itself dirty on every applied event; the renderer only redraws when dirty.
- **Renderer-only display decisions.** Tile slot placement (`TileSlotMap`), team colors
  (`ColorPalette`), localization (`I18n`) live in `raylib_helpers/`, never in the data layer.

## Rendering at a glance

- Orbital observer camera (free-cam with `F`); 3D GLB models for players/eggs/food/stones.
- Click to select any entity → wireframe highlight + tooltip; HUD with map size, uptime, team scores.
- Helper classes in `raylib_helpers/` (one concern each): `EntityRenderer`, `GridRenderer`,
  `TextRenderer`, `TooltipRenderer`, HUD/panel/slider widgets, `SelectionFinder`, `I18n`.

## Protocol extensions

Two non-standard commands the server answers only for GUI clients (AI unaffected):

- `stu` — server uptime, polled ≤ once/sec; HUD shows `Time --:--` if silenced after 3 misses.
- `sse` — server-spawned startup eggs, distinguished from player-laid `enw` eggs.

## Tests

Unit suites over `TcpSocket`, `EventQueue`, `ProtocolParser`, `GameState`, `Args`, `App`,
`TileSlotMap`, `I18n`. E2E is manual against the reference server (`bin/zappy_server`).

```
cmake --build build --target zappy_gui zappy_gui_tests && ./zappy_gui_tests
```
