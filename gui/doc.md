# Zappy GUI Architecture

## Layer Structure

```
TcpSocket → ProtocolParser → EventQueue → GameState → IRenderer
                                             ↓
                                         WorldState
```

**Data flow:**
- Network layer pushes parsed events to queue
- Main loop drains queue, applies events to GameState
- Renderer pulls const ref to GameState each frame

## Components

### network/TcpSocket.hpp
RAII wrapper around POSIX socket with line buffering.

```cpp
class TcpSocket {
    int _fd;
    std::string _recvBuffer;  // accumulates partial lines

public:
    void connect(const std::string& host, int port);
    void send(const std::string& data);
    std::optional<std::string> recvLine();  // returns complete line (ends with \n) or nullopt
    bool poll(int timeout_ms);              // data ready to read?
    ~TcpSocket();                           // close(_fd)
};
```

**Line buffering:** `recvLine()` accumulates socket data until `\n` found. Prevents incomplete message parsing caused by TCP packet boundaries.

### network/ProtocolParser.hpp
Parse GUI protocol lines → Event variant.

```cpp
// Protocol events from server
std::optional<Event> parseEvent(std::string_view line);

// Helper: split line into tokens, index for parsing
std::vector<std::string_view> split(std::string_view, char delim);
```

**Parsing approach:** Manual with token indexing.
- Split line by space once
- Match tokens[0] to event type
- Parse remaining tokens by position
- Return nullopt on malformed input

Example: `"pnw #42 5 7 2 1 TeamA\n"` → `PlayerNew{id:42, x:5, y:7, orientation:E, level:1, team:"TeamA"}`

### core/EventQueue.hpp
Thread-safe queue of parsed events. Optional for single-threaded, but allows future network thread.

```cpp
class EventQueue {
    std::queue<Event> _queue;
    std::mutex _mutex;
public:
    void push(Event e);
    std::optional<Event> pop();
};
```

### core/GameState.hpp
WorldState wrapper + metadata + event application logic.

```cpp
class GameState {
public:
    WorldState world;
    ConnectionStatus status;
    int timeUnit;
    std::string winnerTeam;

    void applyEvent(const Event& e);
};
```

**applyEvent() responsibilities:**
- `msz X Y` → resize world.tiles, set width/height
- `bct X Y q0...q6` → update world.at(x,y) resources
- `tna N` → world.teams.push_back(N)
- `pnw #n X Y O L N` → world.players[n] = Player{...}
- `ppo #n X Y O` → update world.players[n] position/orientation
- `plv #n L` → update world.players[n].level
- `pin #n X Y q0...q6` → update world.players[n].inventory
- `pdi #n` → world.players.erase(n)
- `enw #e #n X Y` → world.eggs[e] = Egg{...}
- `ebo #e` → remove egg, spawn player (handled via pnw)
- `edi #e` → world.eggs.erase(e)
- `pic X Y L #n #n...` → mark tile incantation in progress
- `pie X Y R` → clear incantation, level up if R=1
- `seg N` → winnerTeam = N
- `sgt T` → timeUnit = T

### renderer/IRenderer.hpp
Already exists. Update signature:

```cpp
class IRenderer {
public:
    virtual void init() = 0;
    virtual void render(const GameState& state) = 0;
    virtual bool shouldClose() = 0;
    virtual void shutdown() = 0;
};
```

### renderer/HeadlessRenderer
Text-based state dump for testing. Already exists, update to handle GameState.

### renderer/RaylibRenderer (future)
3D visualization with raylib.
- Camera3D (orthographic top-down initially, orbit later)
- Grid rendering (toroidal wrapping)
- Player models (show orientation, level, TTL bar)
- Resource/egg sprites
- Incantation effects
- Click interaction (tile details, player info)

## Main Loop

```cpp
int main(int argc, char** argv) {
    // Parse args: -p port -h host

    TcpSocket socket;
    socket.connect(host, port);
    socket.send("GRAPHIC\n");

    ProtocolParser parser;
    EventQueue eventQueue;
    GameState gameState;
    HeadlessRenderer renderer(std::cout);  // or RaylibRenderer

    renderer.init();

    while (!renderer.shouldClose()) {
        // Network: non-blocking read
        if (socket.poll(0)) {
            auto line = socket.recv();
            if (line) {
                if (auto event = parser.parse(*line)) {
                    eventQueue.push(*event);
                }
            }
        }

        // Update: drain event queue
        while (auto event = eventQueue.pop()) {
            gameState.applyEvent(*event);
        }

        // Render
        renderer.render(gameState);
    }

    renderer.shutdown();
    return 0;
}
```

## Threading

**Current:** Single-threaded with non-blocking socket poll.

**Future (if needed):**
- Network thread: blocking recv, push to EventQueue
- Main thread: drain EventQueue + render
- Mutex on EventQueue only

Profile before adding complexity.

## Protocol Handshake

```
Client → Server: connect to port
Server → Client: "WELCOME\n"
Client → Server: "GRAPHIC\n"
Server → Client: "msz X Y\n"
Server → Client: "bct ..." (all tiles)
Server → Client: "tna ..." (all teams)
Server → Client: ongoing events...
```

Client must send `msz\n` to request map size if server doesn't auto-send.

## Event Variant

```cpp
// msz X Y\n
struct MapSize { int width, height; };

// bct X Y q0 q1 q2 q3 q4 q5 q6\n
struct TileContent { int x, y; Resources res; };

// tna N\n
struct TeamName { std::string name; };

// pnw #n X Y O L N\n
struct PlayerNew { int id, x, y; Orientation o; int level; std::string team; };

// ppo #n X Y O\n
struct PlayerPosition { int id, x, y; Orientation o; };

// plv #n L\n
struct PlayerLevel { int id, level; };

// pin #n X Y q0 q1 q2 q3 q4 q5 q6\n
struct PlayerInventory { int id, x, y; Resources inv; };

// pdi #n\n
struct PlayerDeath { int id; };

// enw #e #n X Y\n
struct EggNew { int eggId, playerId, x, y; };

// ebo #e\n
struct EggHatch { int eggId; };

// edi #e\n
struct EggDeath { int eggId; };

// pic X Y L #n #n ...\n
struct IncantationStart { int x, y, level; std::vector<int> playerIds; };

// pie X Y R\n (R: 0=fail, 1=success)
struct IncantationEnd { int x, y; bool success; };

// pbc #n M\n
struct Broadcast { int playerId; std::string message; };

// pex #n\n
struct Expulsion { int playerId; };

// pdr #n i\n
struct ResourceDrop { int playerId, resourceId; };

// pgt #n i\n
struct ResourceCollect { int playerId, resourceId; };

// pfk #n\n
struct EggLaying { int playerId; };

// sgt T\n
struct TimeUnit { int value; };

// seg N\n
struct EndGame { std::string winnerTeam; };

// suc\n - server received unknown command from GUI
struct UnknownCommand {};

// sbp\n - server received bad parameter from GUI
struct BadParameter {};

// smg M\n - message from server
struct ServerMessage { std::string message; };

using Event = std::variant<
    MapSize,
    TileContent,
    TeamName,
    PlayerNew,
    PlayerPosition,
    PlayerLevel,
    PlayerInventory,
    PlayerDeath,
    EggNew,
    EggHatch,
    EggDeath,
    IncantationStart,
    IncantationEnd,
    Broadcast,
    Expulsion,
    ResourceDrop,
    ResourceCollect,
    EggLaying,
    TimeUnit,
    EndGame,
    UnknownCommand,
    BadParameter,
    ServerMessage
>;
```

## File Structure

```
gui/
├── src/
│   ├── main.cpp
│   ├── WorldState.hpp           (existing)
│   ├── core/
│   │   ├── GameState.hpp
│   │   ├── GameState.cpp
│   │   ├── EventQueue.hpp
│   │   └── Event.hpp
│   ├── network/
│   │   ├── TcpSocket.hpp
│   │   ├── TcpSocket.cpp
│   │   ├── ProtocolParser.hpp
│   │   └── ProtocolParser.cpp
│   └── renderer/
│       ├── IRenderer.hpp        (existing)
│       ├── HeadlessRenderer.hpp (existing)
│       ├── HeadlessRenderer.cpp (existing)
│       ├── RaylibRenderer.hpp   (future)
│       └── RaylibRenderer.cpp   (future)
├── CMakeLists.txt
└── doc.md
```

## Implementation Tasks (GitHub Issues)

### Core Data Structures
**[CORE-1] Define Event variant types**
- Create `core/Event.hpp`
- Define all protocol event structs with protocol format comments
- Create `Event` variant type
- Estimated: 1-2h

**[CORE-2] Implement EventQueue**
- Create `core/EventQueue.hpp`
- Thread-safe queue with mutex
- `push()` and `pop()` methods
- Estimated: 30min

**[CORE-3] Implement GameState**
- Create `core/GameState.hpp/.cpp`
- Wrap WorldState + metadata (timeUnit, connection status, winner)
- Stub `applyEvent()` method (implement per-event in subsequent tasks)
- Estimated: 1h

**[CORE-4] Implement GameState event handlers**
- Fill out `applyEvent()` for all event types
- Map events → WorldState updates
- Handle edge cases (player/egg not found, etc)
- Estimated: 3-4h

### Network Layer
**[NET-1] Implement TcpSocket wrapper**
- Create `network/TcpSocket.hpp/.cpp`
- RAII socket wrapper
- `connect()`, `send()`, `recvLine()` with line buffering, `poll()`
- Non-blocking recv with `_recvBuffer` accumulation
- Error handling (connection refused, broken pipe, etc)
- Estimated: 2-3h

**[NET-2] Implement ProtocolParser**
- Create `network/ProtocolParser.hpp/.cpp`
- `parseEvent(string_view)` → `optional<Event>`
- Token-based parsing (split by space, index tokens)
- Handle all protocol messages from GUI spec
- Return `nullopt` on malformed input
- Estimated: 4-5h

**[NET-3] Add command serialization**
- Add methods to send GUI→Server commands (msz, bct, mct, tna, ppo, plv, pin, sgt, sst)
- Format strings correctly per protocol
- Estimated: 1-2h

### Integration
**[INT-1] Wire main loop**
- Update `main.cpp` to use all layers
- Parse CLI args (-p port, -h host)
- Connect, send "GRAPHIC\n"
- Main loop: poll → parse → queue → apply → render
- Estimated: 2h

**[INT-2] Update HeadlessRenderer**
- Change `render()` signature to `render(const GameState&)`
- Dump state: map size, player count, teams, resources, etc
- Estimated: 1h

**[INT-3] Optional: Extract App/Gui class**
- Wrap main loop logic in `App` or `Gui` class
- Cleaner separation, easier testing
- Estimated: 1-2h

### Rendering (Future)
**[RENDER-1] Integrate Raylib**
- Uncomment raylib in CMakeLists.txt
- Create `RaylibRenderer.hpp/.cpp`
- Basic window + 3D camera setup
- Estimated: 2h

**[RENDER-2] Implement 3D map rendering**
- Grid rendering (toroidal wrapping visualization)
- Tile resources display
- Camera controls (orbit, zoom)
- Estimated: 4-6h

**[RENDER-3] Implement player/egg rendering**
- Player models (show orientation, level indicator, TTL bar)
- Egg rendering
- Team colors
- Estimated: 3-4h

**[RENDER-4] Add interactive features**
- Click tile → show details
- Click player → show inventory/level
- Time control UI (sst command)
- Estimated: 3-4h

**[RENDER-5] Add animations/effects**
- Incantation animation
- Broadcast visualization
- Death/hatch effects
- Player action indicators (pickup/drop/eject)
- Estimated: 4-6h

## Testing Strategy

HeadlessRenderer allows testing full pipeline without graphics:
- Mock TcpSocket or inject canned protocol strings
- Parse → EventQueue → GameState → HeadlessRenderer dumps state
- Verify state updates match protocol semantics

## Design Decisions

**Why EventQueue instead of direct apply?**
- Decouples network from game logic
- Allows batching multiple events per frame
- Enables future async network thread

**Why GameState wrapper instead of bare WorldState?**
- Encapsulates event application logic
- Stores metadata (timeUnit, connection status, winner)
- WorldState stays pure data structure

**Why manual parsing instead of scanf?**
- Better error handling (malformed input)
- No buffer overflow risk
- Clear token indexing vs format string

**Why single-threaded for now?**
- Protocol simple, network unlikely to bottleneck
- Avoid mutex complexity
- Profile before optimizing

**Why line buffering in TcpSocket?**
- TCP doesn't guarantee message boundaries
- Socket recv can return partial lines ("msz 10 " then "20\n")
- Buffer accumulates until complete line (\n) available
- Parser receives only complete protocol messages

**How handle malformed/unknown commands?**
- Parser returns `optional<Event>` - nullopt on parse failure
- Server error events (suc/sbp/smg) are proper Event types
- Parse failure → log warning, continue (don't crash on bad input)
- Server errors → add to event stream, handle in GameState

**Why App/Gui wrapper class optional?**
- Main loop simple enough to inline in main()
- Wrapper adds testability and cleaner organization
- Not required for MVP, add if codebase grows
