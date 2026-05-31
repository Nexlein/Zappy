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
    ConnectionStatus status;
    int timeUnit;
    std::string winnerTeam;

    void applyEvent(const Event& e);
    bool isDirty() const;
    void clearDirty() const;

private:
    mutable bool dirty = false;
    // ... event handlers
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
Text-based state dump for testing/debugging.

Prints game state updates to stdout when dirty flag set.

### renderer/RaylibRenderer
*(Not yet implemented - future 3D visualization)*

## Main Loop

```cpp
int main(int argc, char** argv) {
    App app(argc, argv);
    if (!app.shouldRun()) {
        return app.exitCode();
    }
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
    IRenderer* renderer = config.headless ? new HeadlessRenderer(...) : ...;

    while (!renderer->shouldClose()) {
        pollAndEnqueue(socket, queue);

        while (auto event = queue.pop()) {
            state.applyEvent(*event);
        }

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

Example:
```bash
# Terminal 1: Server
../bin/zappy_server -p 4242 -x 10 -y 10 -n TeamA TeamB -c 10 -f 1000

# Terminal 2: GUI
./zappy_gui -p 4242 --headless

# Terminal 3: Scripted player
{ echo "TeamA"; sleep 0.5; echo "Forward"; sleep 0.5; echo "Take food"; sleep 2; } | nc localhost 4242
```

Verified: player join, movement, orientation, resource take/drop, inventory updates, player death.

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
│       ├── HeadlessRenderer.hpp/cpp
│       └── RaylibRenderer.hpp/cpp (future)
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
