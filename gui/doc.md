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
- All 24 protocol commands implemented

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

### core/Args
CLI argument parser with validation.

**Flags:**
- `-p port` (required): server port (1-65535)
- `-h machine` (optional): server host (default: localhost)
- `--headless` (optional): use headless renderer
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
- Players: colored cubes (team color), labeled "Player #N" above
- Eggs: colored cubes (team color), labeled "Egg #N" above
- Tiles: grid with per-tile resource indicators (dots scaled by quantity)
- All world coordinates computed via `RenderingHelper::tileToWorld()`

**Selection System**
- Single click: raycast via `SelectionFinder`, selects closest entity (player, egg, or tile), auto-deselects after 5s timer
- Double click (≤300ms): permanent selection, bypasses timer
- Click empty space: clears selection
- Selected entity gets a wireframe highlight (players/eggs) or grid outline (tiles)
- Selection state lives in `SelectionFinder::Selection` (type, id, tileX/Y, timer, permanent flag)

**HUD (top-left)**
- FPS counter (green ≥55, yellow ≥30, red <30)
- Map dimensions
- Current time unit
- Top 5 teams by player count, each in their team color

**Entity Tooltip (top-right)**
- Appears when an entity is selected
- Entity type label colored: Tile=cyan, Player=green, Egg=amber
- Tile: lists non-zero resources
- Player: ID, team name (team color), level, inventory (non-zero resources)
- Egg: ID, team name (team color)
- Built with `TooltipRenderer` builder pattern

**Window**
- Resizable; font sizes scale with screen height (base 600px = 1.0x, clamped 0.5x–2.5x)

### renderer/raylib_helpers

Static helper classes extracted from RaylibRenderer for maintainability:

| Class | Responsibility |
|-------|---------------|
| `RenderingHelper` | World↔screen coordinate conversion (`tileToWorld`) |
| `EntityRenderer` | Draw players, eggs, resources, and their highlights |
| `GridRenderer` | Draw tile grid and tile highlights |
| `TextRenderer` | Draw text anchored to 3D world positions |
| `TooltipRenderer` | Builder-pattern 2D tooltip renderer (multi-line, colored segments, anchored) |
| `SelectionFinder` | Raycast against world entities, returns closest hit |

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
- Broadcast visual feedback (directional wave or floating message)
- Death visual (dissolve, fade, or brief marker on tile)
- Win screen / end game display when a team reaches 6 players at level 8
- Fork visual (egg appearing animation on tile when player forks)
- Level-up visual (burst/flash effect on all players participating in a successful elevation)
- Smooth movement animation — instead of teleporting, interpolate player position/rotation between tiles over the action's time window (Forward=7/f, turn=7/f); animation speed adapts to server frequency so it always fits within the tick budget
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
    TcpSocket socket;
    socket.connect(config.machine, config.port);
    socket.send("GRAPHIC\n");

    EventQueue queue;
    IRenderer* renderer = config.headless ? new HeadlessRenderer(...) : new RaylibRenderer(...);

    while (!renderer->shouldClose()) {
        pollAndEnqueue(socket, queue);
        while (auto event = queue.pop()) state.applyEvent(*event);
        renderer->render(state);
    }

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

**Unit tests:** 102 tests across all components
- TcpSocket (11 tests)
- EventQueue (7 tests)
- ProtocolParser (36 tests)
- GameState (24 tests)
- Args (21 tests)
- App (2 tests)
- Stub (1 test)

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
│   │   └── App.hpp/cpp
│   ├── network/
│   │   ├── TcpSocket.hpp/cpp
│   │   └── ProtocolParser.hpp/cpp
│   └── renderer/
│       ├── IRenderer.hpp
│       ├── ARenderer.hpp
│       ├── HeadlessRenderer.hpp/cpp
│       ├── RaylibRenderer.hpp/cpp
│       └── raylib_helpers/
│           ├── RenderingHelper.hpp/cpp
│           ├── EntityRenderer.hpp/cpp
│           ├── GridRenderer.hpp/cpp
│           ├── TextRenderer.hpp/cpp
│           ├── TooltipRenderer.hpp/cpp
│           └── SelectionFinder.hpp/cpp
├── CMakeLists.txt
└── doc.md
```

## Implementation Notes

**Why dirty flag?**
Server sends 100+ events on initial handshake, then sparse updates. Dirty flag prevents render spam.

**Why line buffering?**
TCP doesn't guarantee message boundaries. `recv()` can return partial lines. Buffer accumulates until `\n`.

**Why thread-safe EventQueue?**
Optional for now (single-threaded), but enables future async network thread without refactoring.

**Why Args class separate from App?**
Single responsibility. Args handles parsing/validation, App handles orchestration. Easier to test.

**Why static helper classes for renderer?**
RaylibRenderer grew large. Extracting stateless drawing concerns (grid, entities, text, tooltips) into static classes keeps each file focused and makes them independently testable.

**Why blend elliptic+spherical orbit?**
Pure circular orbit makes non-square maps look skewed. Pure elliptic stretches too far on wide maps. 50% blend gives a natural feel across all aspect ratios.
