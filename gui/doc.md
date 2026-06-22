# Zappy GUI - Implementation Documentation

## Architecture Overview

```
TcpSocket → ProtocolParser → EventQueue → GameState → Renderer
                                             ↓
                                         WorldState
```

**Data flow:**
- Network layer receives lines, parses into events, pushes to queue
- Main loop drains queue, applies events to GameState
- Renderer reads GameState and displays when dirty flag set

## Components

### network/TcpSocket
RAII TCP socket wrapper with line buffering and non-blocking I/O.

**Key features:**
- Line buffering: accumulates recv() data until `\n` found
- Non-blocking mode with `poll()` for timeout control
- RAII cleanup: socket closed in destructor

### network/ProtocolParser
Static factory that parses protocol lines into Event variants.

**Implementation:**
- Token-based parsing (split by space, match command, parse args)
- Returns `std::optional<Event>` (nullopt on malformed input)
- Supports multi-word fields (team names, broadcast messages)
- All 24 standard protocol commands implemented, plus 2 custom extensions (see below)

### Custom Protocol Extensions

Two commands added on top of the standard GUI protocol. Non-disruptive: server handles them only for GUI clients, AI clients are unaffected.

| Command | Direction | Format | Description |
|---------|-----------|--------|-------------|
| `stu` | GUI→Server, Server→GUI | `stu` / `stu T` | GUI polls each frame; server replies with uptime `T` in seconds |
| `sse` | Server→GUI | `sse #e N X Y` | Server-spawned egg at startup; `#e` = egg ID, `N` = team name, `X Y` = tile position |

**`stu`:** Enables the uptime display in the HUD. GUI sends `stu\n` at most once per real-time second (`App::_trySendStu`). If the server fails to respond 3 consecutive times, polling is silenced for the session and the HUD shows `Time --:--`. `GameState::serverUptimeSeconds` stores the last received value; `GameState::receivedStuResponse` is set to `true` on receipt so `_trySendStu` can detect a live response.

**`sse`:** Each team starts with 10 server-spawned slots, represented as pre-placed eggs. The server sends one `sse` per egg at connection time. Parsed into `ServerSpawnedEgg` event, handled identically to `enw` eggs in `GameState`.

### core/EventQueue
Thread-safe queue for parsed events.

**Purpose:**
- Decouples network from game logic
- Allows batching events per frame
- Thread-safe for future async network option

### core/GameState
WorldState wrapper with event application logic and dirty flag.

**Structure:**
```cpp
class GameState {
public:
    WorldState world;
    int timeUnit;
    std::string winnerTeam;
    float tileSize = 1.0f;  // used when constructing behaviors

    void applyEvent(const Event& e);
    bool isDirty() const;
    void clearDirty() const;

private:
    mutable bool dirty = false;
};
```

**Dirty flag optimization:**
- Set to `true` on every `applyEvent()`
- Renderer checks before rendering, clears after
- Prevents redundant renders when no state changes

**Behavior creation:**
- `applyPlayerNew` initializes `player.visual.pos` to spawn world position
- `applyPlayerPosition` clears existing behaviors and pushes a `MoveBehavior` (duration = `7.0f / timeUnit`)

### core/behaviors

Visual behavior system. Each `IBehavior` subclass owns a piece of per-entity visual state and ticks every frame via `VisualState::update(dt)`.

```
IBehavior (pure virtual: update, isDone, getDuration, minDuration)
├── MoveBehavior         — smoothstep lerp of visual.pos between tiles, handles toroidal wrap
├── TurnBehavior         — smoothstep lerp of visual.angle with 0/360 wraparound
└── ADrawableBehavior    — abstract base for visual effects; owns mutable _particles + _lines, exposes getParticles()/getLines()
    ├── DeathBehavior        — smoothstep shrink of visual.scale (1.0→0.05) + omnidirectional particle burst (10 ticks / freq)
    ├── LevelUpBehavior      — staggered upward particle burst, yellow/orange palette (20 ticks / freq)
    ├── BroadcastBehavior    — expanding ring wave (64-point circle of particles + 30 scatter seeds) centered on broadcaster, fades over 7 ticks / freq
    └── ForkBehavior         — scale-up animation on egg spawn, particle burst at full size (10 ticks / freq)
```

**Design:**
- Logical state (`x`, `y`, `orientation`) stays server-authoritative in `Player`
- Visual state (`visual.pos`, `visual.angle`, `visual.scale`) is driven by behaviors
- Particles and lines are owned per-behavior in `ADrawableBehavior` — isolated, no cross-contamination
- `applyPlayerPosition` erases only `MoveBehavior`/`TurnBehavior` — other behaviors (e.g. `LevelUpBehavior`, `BroadcastBehavior`) survive movement events
- New behaviors: subclass `IBehavior` or `ADrawableBehavior`, push onto `player.visual.behaviors` (or `egg.visual.behaviors`) from relevant `GameState::apply*`, add `.cpp` to both `gui/CMakeLists.txt` and `tests/CMakeLists.txt`

**Death flow:**
- `applyPlayerDeath` moves `Player` from `world.players` → `world.dyingPlayers`, pushes `DeathBehavior` onto the settled entry's visual
- `DeathBehavior` spawns 14 particles (immediate, no delay), updates shrink + physics each frame
- Renderer draws dying players (scaled) + behavior particles via `_drawBehaviorParticles`, then calls `world.purgeDyingPlayers()`
- `world.dyingPlayers` and `purgeDyingPlayers()` are `mutable` — visual-only lifecycle

**LevelUp flow:**
- `applyPlayerLevel` pushes `LevelUpBehavior` alongside existing behaviors (no clear)
- Particles staggered with random delay up to 60% of duration; each activates at `_visual.pos` at delay expiry (follows player movement)
- Behavior removes itself and clears `_particles` when done

**Broadcast flow:**
- `applyBroadcast` pushes `BroadcastBehavior` onto the broadcasting player's visual
- Ring expands outward from player position; map half-dimensions passed in for toroidal clamping
- 30 scatter seeds add jitter to the ring for a natural look

**Fork flow:**
- `applyPlayerFork` / egg spawn pushes `ForkBehavior` onto the new egg's visual
- Egg scales from 0 to full size over duration, then fires a particle burst and marks itself done

**Renderer drawable draw:**
- `_drawBehaviorParticles(visual)` — iterates `visual.behaviors`, casts to `ADrawableBehavior*`, draws active particles via `DrawSphere` and lines via `DrawLine3D`

### core/Args
CLI argument parser with validation.

**Flags:**
- `-p port` (required): server port (1-65535)
- `-h machine` (optional): server host (default: localhost)
- `--headless` (optional): use headless renderer
- `--dev true|false` (optional): enable dev HUD (FPS, time unit, port, machine); default false
- `--language english|french` (optional): UI language at launch; default english
- `--help`: show usage

**Exit codes:**
- 0: help requested or success
- 84: parse error

### core/App
Main application orchestrator.

**Responsibilities:**
- Construct with `argc, argv`
- `shouldRun()` / `exitCode()` for flow control
- `run()`: main loop (poll → parse → apply → render)
- `pollAndEnqueue()`: helper for network event collection

### renderer/HeadlessRenderer
Text-based state dump for testing/debugging. Prints game state updates to stdout when dirty flag set.

### renderer/RaylibRenderer
3D visualization using raylib. Core rendering and interaction implemented; visual polish and advanced features still in progress.

**Camera: Orbital Observer**
- Orbits around map center at fixed height
- Controls: `Q`/`A` or Left/Right arrows to rotate
- Orbit path is a blend of elliptic and spherical trajectories to handle non-square maps gracefully
- Camera height scales with orbit radius to maintain viewing angle

**3D Rendering**
- Players: 3D GLB model (rimuru slime, `gui/assets/rimuru.glb`) tinted per team via `ColorPalette::getSlimePalette()`, falls back to colored cube if model fails to load
- Eggs: 3D GLB model (`gui/assets/egg.glb`) with team-colored inner layer, random stable Y rotation set at spawn
- Tiles: grid with per-tile resource indicators (spheres scaled logarithmically by quantity, stable random positions per tile)
- All world coordinates computed via `RenderingHelper::tileToWorld()`

**Selection System**
- Single click: raycast via `SelectionFinder`, selects closest entity (player, egg, or tile); selection is permanent
- Click empty space: clears selection
- Selected entity gets a wireframe highlight (players/eggs) or grid outline (tiles)
- Selection state lives in `SelectionFinder::Selection` (type, id, tileX/Y)
- Egg raycasts apply the same tile slot offset as the visual position so hitbox matches what is drawn

**HUD (top-left)**
- Map dimensions
- Server uptime (`Xh Ym Zs`, hours/minutes omitted when zero; `Time --:--` if unavailable)
- Top 5 teams by player count, each in their team color
- **Dev mode only** (`--dev true` or `F3` toggle): FPS counter (green ≥55, yellow ≥30, red <30), time unit, port, machine

**Window persistence**
- On `shutdown()`, window state (size, position, monitor, fullscreen) is saved to `_savedWindow`
- On `init()` after reconnect, `_savedWindow` is restored — window reopens exactly where it closed

**Entity Tooltip (top-right)**
- Appears when an entity is selected
- Entity type label colored: Tile=cyan, Player=green, Egg=amber
- Tile: lists non-zero resources
- Player: ID, team name (team color), level, inventory (non-zero resources)
- Egg: ID, team name (team color)
- Built with `TooltipRenderer` builder pattern

**Keybinds**
- `Q`/`A` or Left/Right arrows: rotate camera
- `L`: cycle UI language (EN↔FR)
- `F3`: toggle dev HUD on/off at runtime

**Window**
- Resizable; font sizes scale with screen height (base 600px = 1.0x, clamped 0.5x–2.5x)

### renderer/raylib_helpers

Static helper classes extracted from RaylibRenderer for maintainability:

| Class | Responsibility |
|-------|---------------|
| `ColorPalette` | Team color palette + slime material palettes. Single source of truth for all entity coloring |
| `RenderingHelper` | World↔screen coordinate conversion (`tileToWorld`) |
| `EntityRenderer` | Draw players, eggs, resources, and their highlights |
| `GridRenderer` | Draw tile grid and tile highlights |
| `TextRenderer` | Draw text anchored to 3D world positions |
| `TooltipRenderer` | Builder-pattern 2D tooltip renderer (multi-line, colored segments, anchored) |
| `SelectionFinder` | Raycast against world entities, returns closest hit; takes `TileSlotMap` to apply egg slot offsets |
| `TileSlotMap` | Per-tile slot occupancy: assigns and tracks stable visual positions for resources and eggs (8 slots/tile) |
| `I18n` | Compile-time EN/FR string table; O(1) lookup via static constexpr 2D array indexed by `[Language][Key]` |

**TileSlotMap — tile slot system**

Resources and eggs are displayed at one of 8 fixed slots per tile rather than at tile center or random positions. This prevents visual overlap with players (always at tile center) and with each other.

Slot layout (offsets are fractions of `tileSize`):

| Index | Type   | dx    | dz    |
|-------|--------|-------|-------|
| 0–3   | corner | ±0.35 | ±0.35 |
| 4–5   | edge   | 0.00  | ±0.35 |
| 6–7   | edge   | ±0.35 | 0.00  |

- Resources and eggs share the same per-tile occupancy — they cannot land on the same slot
- Slot assignment is renderer-only; the data layer (`WorldState`, `GameState`) is unchanged
- `updateResourceSlots(x, y, res)` — called each frame; assigns a slot on 0→nonzero transition, releases on nonzero→0; returns `std::array<int,7>` of slot indices (-1 = not present)
- `syncEggs(eggs)` — diffs current egg map against known set; assigns slots to new eggs, releases slots for gone eggs
- Overflow: if all 8 slots full, new entity picks `rand() % 8` (visual overlap accepted, no crash)
- Slot selection is random among free slots (`rand() % freeCount`) for natural spread

**I18n — compile-time localization**

All UI strings are stored as a static `constexpr` 2D array indexed by `[Language][Key]`. Zero allocation, O(1) lookup, no file I/O — deliberately chosen over a config file because languages are a compile-time concern and a missing file is an unnecessary failure mode.

- `I18n::get(Key)` — returns `const char*` for current language
- `I18n::resourceName(int index)` — resource name by index 0–6, matching `Resources[]` ordering
- `I18n::setLanguage(Language)` / `getLanguage()` — runtime language switch
- Language set at startup via `--language` arg; toggled live with `L` key (EN↔FR cycle)
- French typography: space before `:` in all FR label prefixes

**TooltipRenderer builder API:**
```cpp
TooltipRenderer::create()
    .setAnchor(Anchor::TopRight)
    .setBackgroundColor(color)
    .setBackgroundAlpha(alpha)
    .setBorderColor(color)
    .setBorderThickness(px)
    .setPadding(px)
    .setFontSize(px)
    .addLine("text", color)
    .addColoredText({"seg1", "seg2"}, {color1, color2})
    .draw({x, y});
```

## Planned / Ideas

- OBJ model support for players and eggs (replace placeholder cubes)
- Improved resource visuals (3D models or icons instead of dots)
- Map aesthetics: skybox, ground texture, decorative elements
- Incantation visual feedback (glow, particle effect, frozen player indicator)
- Win screen / end game display when a team reaches 6 players at level 8
- Incantation progress bar on the tile (300/f duration, visual countdown until success or failure)
- Player history trail — faint path showing recent movement of selected player (style TBD based on overall visual direction)
- Spectate / follow mode — camera locks onto and follows a selected player (3rd person or overhead, TBD)

## Main Loop

```cpp
int main(int argc, char** argv) {
    App app(argc, argv);
    if (!app.shouldRun()) return app.exitCode();
    app.run();
    return 0;
}
```

Inside `App::run()`:
```cpp
void App::run() {
    auto socket = make_unique<TcpSocket>();
    _connectWithRetry(*socket, host, port);  // exponential backoff, 5 attempts

    IRenderer* renderer = config.headless ? new HeadlessRenderer(...) : new RaylibRenderer(...);
    renderer->setDevMode(config.dev, config.port, config.machine);
    renderer->init();
    _rendererActive = true;

    while (!renderer->shouldClose()) {
        try {
            socket->send("mct\n");
            _trySendStu(*socket);  // throttled: once/s, silenced after 3 missed responses
            pollAndEnqueue(*socket, queue);
            while (auto event = queue.pop()) state.applyEvent(*event);
            renderer->setState(state);
            renderer->handleInput();
            renderer->render();
        } catch (const TcpException&) {
            renderer->shutdown();
            _rendererActive = false;
            // reconnect with retry; if fails, break
            state = {}; queue.clear(); _resetStuState();
            renderer->init();
            _rendererActive = true;
        }
    }

    if (_rendererActive) renderer->shutdown();
    delete renderer;
}
```

## Protocol Handshake

```
Client → Server: TCP connect
Server → Client: "WELCOME\n"
Client → Server: "GRAPHIC\n"
Server → Client: "msz X Y\n" + "bct ..." (all tiles) + "tna ..." (all teams)
Server → Client: ongoing events...
```

## Testing

**Unit tests:** 117 tests across 8 suites
- TcpSocket (11 tests)
- EventQueue (7 tests)
- ProtocolParser (36 tests)
- GameState (24 tests)
- Args (21 tests)
- App (2 tests)
- TileSlotMap (11 tests)
- I18n (6 tests) — including resource name index ordering, EN/FR parity check

**E2E verification:** Manual testing against reference server (`bin/zappy_server`)

```bash
# Terminal 1: Server
../bin/zappy_server -p 4242 -x 10 -y 10 -n TeamA TeamB -c 10 -f 1000

# Terminal 2: GUI
./zappy_gui -p 4242

# Terminal 3: Scripted player
{ echo "TeamA"; sleep 0.5; echo "Forward"; sleep 0.5; echo "Take food"; sleep 2; } | nc localhost 4242
```

## File Structure

```
gui/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── WorldState.hpp
│   │   ├── Resources.hpp
│   │   ├── Orientation.hpp
│   │   ├── Event.hpp
│   │   ├── EventQueue.hpp/cpp
│   │   ├── GameState.hpp/cpp
│   │   ├── Args.hpp/cpp
│   │   ├── App.hpp/cpp
│   │   └── behaviors/
│   │       ├── IBehavior.hpp
│   │       ├── ADrawableBehavior.hpp
│   │       ├── MoveBehavior.hpp/cpp
│   │       ├── TurnBehavior.hpp/cpp
│   │       ├── DeathBehavior.hpp/cpp
│   │       ├── LevelUpBehavior.hpp/cpp
│   │       ├── BroadcastBehavior.hpp/cpp
│   │       └── ForkBehavior.hpp/cpp
│   ├── network/
│   │   ├── TcpSocket.hpp/cpp
│   │   └── ProtocolParser.hpp/cpp
│   └── renderer/
│       ├── IRenderer.hpp
│       ├── ARenderer.hpp
│       ├── HeadlessRenderer.hpp/cpp
│       ├── RaylibRenderer.hpp/cpp
│       └── raylib_helpers/
│           ├── ColorPalette.hpp/cpp
│           ├── RenderingHelper.hpp/cpp
│           ├── EntityRenderer.hpp/cpp
│           ├── GridRenderer.hpp/cpp
│           ├── TextRenderer.hpp/cpp
│           ├── TooltipRenderer.hpp/cpp
│           ├── SelectionFinder.hpp/cpp
│           ├── TileSlotMap.hpp/cpp
│           └── I18n.hpp/cpp
├── CMakeLists.txt
└── doc.md
```

## Implementation Notes

**CMake source list is manual.**
New `.cpp` files must be explicitly added to `GUI_SOURCES` in `gui/CMakeLists.txt` — there is no glob. Easy to forget when adding helper classes.

**Why dirty flag?**
Server sends 100+ events on initial handshake, then sparse updates. Dirty flag prevents render spam.

**Why line buffering?**
TCP doesn't guarantee message boundaries. `recv()` can return partial lines. Buffer accumulates until `\n`.

**Why thread-safe EventQueue?**
Optional for now (single-threaded), but enables future async network thread without refactoring.

**Why Args class separate from App?**
Single responsibility. Args handles parsing/validation, App handles orchestration. Easier to test.

**GLB model material mutation:**
Raylib shares a single `Model` instance across all draw calls. Tinting a material slot mutates it globally. Fix: store base material colors on `init()`, restore after each `DrawModelEx` via `EntityRenderer::_restoreModelBaseColors`. Player model has 6 material slots, egg model has 2.

**Why static helper classes for renderer?**
RaylibRenderer grew large. Extracting stateless drawing concerns (grid, entities, text, tooltips) into static classes keeps each file focused and makes them independently testable.

**Why blend elliptic+spherical orbit?**
Pure circular orbit makes non-square maps look skewed. Pure elliptic stretches too far on wide maps. 50% blend gives a natural feel across all aspect ratios.

**Why tile slots in the renderer, not the data layer?**
Slot positions are purely visual — the server never communicates them and they carry no game semantics. `WorldState` represents what the server says happened; slot assignment is a display decision each renderer can make independently. `TileSlotMap` therefore lives in `raylib_helpers/` and is invisible to `HeadlessRenderer`.

**Why I18n as a constexpr array instead of a config file?**
Languages are a compile-time concern. A config file adds a runtime failure mode (file missing, bad path) for something that never changes without a recompile. The 2D array gives O(1) lookup with zero allocation — straightforward to extend (add a language = add a column, add a string = add a row).

**Why `Oeuf` not `Œuf` in French?**
Raylib's default font is ASCII-only. The `Œ` ligature renders as `?` at runtime. Accented letters like `É`, `é`, `à` work fine as they fall in the extended Latin range the bundled font covers.
