# Zappy Server - Implementation Documentation

## Protocol Reference

### Server ↔ AI

AI clients authenticate with a team name, then send commands one at a time (queue max: 10). All responses are terminated with `\n`.

**Handshake**

| Direction | Message | Description |
|-----------|---------|-------------|
| S→C | `WELCOME` | Sent immediately on connect |
| C→S | `<team_name>` | Client sends team name to join |
| S→C | `<slots>` | Remaining egg count for that team |
| S→C | `<w> <h>` | Map dimensions |
| S→C | `ko` | Team unknown or no eggs left — connection closed |

**AI commands (C→S) and responses (S→C)**

| Command | Response | Delay (time units) | Description |
|---------|----------|--------------------|-------------|
| `Forward` | `ok` | 7 | Move one tile in current direction |
| `Right` | `ok` | 7 | Turn 90° clockwise |
| `Left` | `ok` | 7 | Turn 90° counter-clockwise |
| `Look` | `[tile tile ...]` | 7 | Tiles visible in front (level-dependent radius) |
| `Inventory` | `[food N, linemate N, ...]` | 1 | Player's current resources |
| `Broadcast <msg>` | `ok` | 7 | Send message to all players |
| `Connect_nbr` | `<n>` | — | Remaining slots for own team (immediate) |
| `Fork` | `ok` | 42 | Lay an egg (adds a team slot) |
| `Eject` | `ok` / `ko` | 7 | Push all players off current tile |
| `Take <resource>` | `ok` / `ko` | 7 | Pick up a resource from current tile |
| `Set <resource>` | `ok` / `ko` | 7 | Drop a resource on current tile |
| `Incantation` | `Elevation underway` / `ko` | 300 | Start elevation ritual |
| *(unknown)* | `ko` | — | Any unrecognised command |

**Incantation result (sent after delay)**

| Message | Description |
|---------|-------------|
| `Current level: <n>` | Ritual succeeded — player is now level n |
| `ko` | Ritual failed (conditions no longer met) |
| `dead` | Player starved — connection will be closed |

---

### Server ↔ GUI

GUI clients authenticate with the literal team name `GRAPHIC`. On connect the server pushes a full world snapshot, then broadcasts all state changes in real time.

**Handshake**

| Direction | Message | Description |
|-----------|---------|-------------|
| S→C | `WELCOME` | Sent immediately on connect |
| C→S | `GRAPHIC` | GUI identification |
| S→C | `msz <w> <h>` | Map size (first snapshot line) |
| S→C | `bct <x> <y> <food> <lin> <der> <sib> <men> <phi> <thy>` | One per tile (w×h lines) |
| S→C | `tna <name>` | One per team |
| S→C | `sgt <freq>` | Current server frequency |
| S→C | `pnw / sse / enw` | One per existing player / egg |

**GUI query commands (C→S) and responses (S→C)**

| Command | Response | Description |
|---------|----------|-------------|
| `msz` | `msz <w> <h>` | Map dimensions |
| `bct <x> <y>` | `bct <x> <y> <resources>` | Resources on one tile |
| `mct` | `bct ...` × (w×h) | Resources on all tiles |
| `tna` | `tna <name>` × teams | All team names |
| `ppo #<id>` | `ppo #<id> <x> <y> <ori>` | Player position & orientation |
| `plv #<id>` | `plv #<id> <level>` | Player level |
| `pin #<id>` | `pin #<id> <x> <y> <resources>` | Player inventory |
| `sgt` | `sgt <freq>` | Current frequency |
| `sst <freq>` | `sst <freq>` *(broadcast)* | Set frequency |
| *(unknown)* | `suc` | Unknown command |
| *(bad args)* | `sbp` | Bad parameters |

**Real-time broadcasts (S→C, sent to all GUI clients)**

| Message | Trigger |
|---------|---------|
| `pnw #<id> <x> <y> <ori> <lvl> <team>` | Player connects |
| `ppo #<id> <x> <y> <ori>` | Player moves |
| `plv #<id> <lvl>` | Player levels up |
| `pin #<id> <x> <y> <resources>` | Player inventory changes |
| `pex #<id>` | Player ejected |
| `pbc #<id> <msg>` | Player broadcasts a message |
| `pic <x> <y> <lvl> #<id> ...` | Incantation starts |
| `pie <x> <y> <result>` | Incantation ends (1=success, 0=fail) |
| `pfk #<id>` | Player forks (egg laid) |
| `pdr #<id> <resource>` | Player drops resource |
| `pgt #<id> <resource>` | Player picks up resource |
| `pdi #<id>` | Player dies |
| `enw #<egg> #<player> <x> <y>` | Egg laid by a player |
| `ebo #<egg>` | Egg hatches (player connects from it) |
| `edi #<egg>` | Egg dies (starved) |
| `sgt <freq>` | Frequency changed |
| `seg <team>` | Game over — team won |
| `smg <msg>` | Server message |

---

### Bonus commands (extensions beyond the base protocol)

These commands are not in the original specification. They were added as bonuses.

**`stu` — server uptime (wall seconds + game ticks)**

| Direction | Message | Description |
|-----------|---------|-------------|
| C→S | `stu` | GUI requests uptime |
| S→C | `stu <seconds> <ticks>` | Integer seconds since server start, and total game ticks |

`seconds` is wall-clock and freq-independent. `ticks` is game time: the integral
of frequency over time (see `core/GameClock`), so it tracks `sst` changes. A mid-game
speed-up leaves `seconds` unchanged but accelerates `ticks`.

**`gtt <team>` — when a team joined**

| Direction | Message | Description |
|-----------|---------|-------------|
| C→S | `gtt <team>` | GUI asks when `<team>`'s first player joined |
| S→C | `gtt <team> <seconds> <ticks>` | Uptime (seconds) and ticks at that team's first join |
| S→C | `gtt <team> -1 -1` | The team has never joined |

Lets the GUI build a win screen: pair `gtt <winner>` (from the `seg` winner) with a
live `stu` to show both "joined at X" and "took Y to win", in seconds and ticks. The
snapshot is taken on the team's **first** player join and never overwritten by later
joins, forks, deaths or respawns.

**`sse` — initial egg notification**

| Direction | Message | Description |
|-----------|---------|-------------|
| S→C | `sse #<egg> <team> <x> <y>` | Sent during GUI snapshot for eggs that existed at server start (not player-laid). Distinguishes initial eggs from `enw` (player-laid eggs). |

---

## Architecture Overview

```
Listener
   │  accept()
   ▼
ClientManager ──── poll() ────► Scheduler::tick()
   │                                    │
   │  readable fds                      │  fire expired events
   ▼                                    ▼
Connection::nextLine()         [callbacks: action complete,
   │                            starvation, resource spawn,
   │  raw command string        incantation end, egg hatch...]
   ▼
CommandDispatcher
   │  dispatch by client type
   ├──► AiParser  ──► handler(World&, Player&)
   └──► GuiParser ──► handler(World&)
              │
              │  world mutates
              ▼
           World ──► GuiNotifier ──► broadcast to all GUI connections
```

**Data flow:**
- `ClientManager` wraps `poll()`, manages all fds (listener + connections)
- Each `Connection` buffers reads/writes, yields complete lines
- `CommandDispatcher` routes lines to AI or GUI handlers based on client type
- Handlers mutate `World` (pure game state, no network knowledge)
- `GuiNotifier` observes mutations and pushes GUI protocol events to all GUI clients
- `Scheduler` (priority queue of timed events) drives all deferred game logic
- `poll()` timeout = `Scheduler::msUntilNext()` — no active waiting ever

## Components

### network/Listener
RAII wrapper around the server accept socket.

**Responsibilities:**
- Bind and listen on the configured port
- `accept()` → returns fd for new connection
- Provides fd for inclusion in `poll()` set

### network/Connection
One instance per connected client fd. Buffers I/O, tracks client state.

```cpp
class Connection {
    int _fd;
    std::string _readBuf;
    std::string _writeBuf;
    ClientType _type;    // PENDING | AI | GUI
    int _playerId = -1;  // set after AI auth
public:
    std::optional<std::string> nextLine();  // consume from readBuf
    void queueWrite(const std::string& msg);
    void flushWrite();   // called when fd is writable in poll
    void appendRead(std::string_view data);
};
```

**Why per-connection buffers:**
TCP doesn't guarantee message boundaries. `recv()` can return partial lines or multiple lines at once. Each connection accumulates raw bytes until `\n` is found.

### network/ClientManager
Owns all connections, manages the `pollfd[]` array.

**Responsibilities:**
- Add/remove connections
- Call `poll(msUntilNext)` each iteration
- Dispatch readable data to connections, flush writable connections
- Return list of complete lines received, tagged by connection id

### core/Scheduler
Priority queue of timed callbacks. The spine of all game timing.

```cpp
struct TimedEvent {
    std::chrono::steady_clock::time_point fireAt;
    std::function<void()> callback;
    bool operator>(const TimedEvent& o) const { return fireAt > o.fireAt; }
};

class Scheduler {
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<>> _queue;
public:
    void schedule(std::chrono::milliseconds delay, std::function<void()> cb);
    void tick();               // fire all events where fireAt <= now
    int msUntilNext() const;   // fed directly to poll() as timeout
    void rescale(float ratio); // used on sst: rescales all pending durations
};
```

**Action timing:** every command has a duration `action / f`. On command start, schedule a callback at `now + duration`. The callback executes the result and dequeues the next command for that player.

**`sst` handling:** when GUI changes `f`, call `Scheduler::rescale(oldF / newF)`. This adjusts all pending fire times. O(n log n) rebuild — acceptable since `sst` is infrequent.

### core/GameClock
Single owner of frequency and game time. Owned by `CommandDispatcher`.

```cpp
class GameClock {
public:
    struct Stamp { std::chrono::microseconds elapsed; double ticks; };
    explicit GameClock(int freq);
    int freq() const;
    std::chrono::microseconds elapsed() const;   // wall time since boot
    double ticks() const;                         // game time since boot
    float setFreq(int newFreq);                   // banks ticks, returns rescale ratio
    void recordJoin(const std::string& team);     // write-once stamp
    std::optional<Stamp> joinOf(const std::string& team) const;
};
```

**Why ticks aren't counted.** The server is wall-clock driven (no fixed-step loop),
so there is no per-tick counter to increment. A tick is `1/freq` seconds, so total
ticks are the **integral of frequency over time**: `ticks() = bankedTicks + secondsSinceLastChange * freq`.
`setFreq` banks `elapsed * oldFreq` before switching, so the count stays correct across
any number of `sst` changes. Multiplying total elapsed time by the *final* freq (the old
bug) is wrong the moment frequency ever changed.

**Per-team join stamps.** `recordJoin` is called once on each team's first AI join
(via the `CommandDispatcher` promotion hook). It captures `{elapsed, ticks}` and is
write-once, so win timing can be measured from the team's entry instead of from server
boot. Feeds `gtt` and the GAME OVER banner.

### core/World
Pure game state + logic. No network, no scheduling — those are the server layer's job.

```cpp
class World {
public:
    int width, height;
    std::vector<Tile> tiles;
    std::unordered_map<int, Player> players;
    std::unordered_map<int, Egg> eggs;
    std::vector<Team> teams;

    Tile& at(int x, int y);

    // Game logic — mutate state, return events for GUI notification
    void movePlayer(int playerId, ...);
    void spawnResources();
    bool startIncantation(int playerId);
    // ...
};
```

**Why pure (no network calls):** testable in isolation. The server layer observes results and calls `GuiNotifier`.

### core/Player (server-side)
Richer than the GUI's Player — owns command queue and active action state.

```cpp
struct Player {
    int id;
    int connectionId;
    int x, y;
    Orientation orientation;
    int level;
    std::string team;
    Resources inventory;
    bool incanting = false;

    std::deque<std::string> commandQueue; // max 10
    bool hasActiveAction = false;
};
```

**Command buffering:** clients may send up to 10 commands without waiting for responses. When current action completes, dequeue next and schedule its timer.

### core/Tile

```cpp
struct Tile {
    Resources resources;
    std::vector<int> playerIds;
    std::vector<int> eggIds;
};
```

### core/Resources
Identical concept to GUI — 7 resource types (food + 6 stones) as a simple struct with named fields.

### core/Args
CLI argument parser with validation.

**Flags:**
- `-p port` (required): listening port
- `-x width` (required): map width
- `-y height` (required): map height
- `-n name1 name2 ...` (required): team names (`GRAPHIC` is reserved, rejected)
- `-c clientsNb` (required): initial slots per team
- `-f freq` (required): time unit frequency (default 100)

**Exit codes:**
- 0: `--help`
- 84: parse error

### core/Server
Top-level orchestrator. Owns everything, runs the loop.

```cpp
class Server {
    Args _args;
    World _world;
    Scheduler _scheduler;
    ClientManager _clients;
    CommandDispatcher _dispatcher;
    GuiNotifier _guiNotifier;
public:
    Server(int argc, char** argv);
    bool shouldRun() const;
    int exitCode() const;
    void run();
};
```

### protocol/AiParser
Parses AI client command strings into typed structs (mirror of GUI's `ProtocolParser`).

- Token-based parsing, returns `std::optional<AiCommand>` (nullopt on malformed)
- All 12 AI commands: `Forward`, `Right`, `Left`, `Look`, `Inventory`, `Broadcast`, `Connect_nbr`, `Fork`, `Eject`, `Take`, `Set`, `Incantation`

### protocol/GuiParser
Parses GUI client request strings.

- GUI-to-server commands: `msz`, `bct X Y`, `mct`, `tna`, `ppo #n`, `plv #n`, `pin #n`, `sgt`, `sst T`, `stu`, `gtt <team>`

### protocol/Serializer
Generates wire strings from typed events (inverse of GUI's `ProtocolParser`).

```cpp
class Serializer {
public:
    static std::string mapSize(int w, int h);
    static std::string tileContent(int x, int y, const Resources& r);
    static std::string playerNew(const Player& p);
    static std::string playerDeath(int id);
    static std::string incantationStart(int x, int y, int level, const std::vector<int>& ids);
    // ... all 24 GUI protocol messages
};
```

### game/CommandDispatcher
Routes received lines to the correct handler based on `Connection::_type`.

- `PENDING` → handshake handler (expects team name next)
- `AI` → `AiParser` → command handler map
- `GUI` → `GuiParser` → GUI request handler

**Handler map (AI):**
```cpp
using AiHandler = std::function<void(World&, Player&, GuiNotifier&, Scheduler&)>;
std::unordered_map<std::string, AiHandler> _aiHandlers;
```

### game/GuiNotifier
Holds references to all GUI connections. Called by the server layer after world mutations.

```cpp
class GuiNotifier {
    ClientManager& _clients;
public:
    void broadcast(const std::string& msg);  // all GUI connections
    void send(int guiConnectionId, const std::string& msg);

    // Typed helpers
    void onPlayerNew(const Player& p);
    void onPlayerDeath(int id);
    void onTileChanged(int x, int y, const Resources& r);
    void onIncantationStart(int x, int y, int level, const std::vector<int>& ids);
    // ...
};
```

### logging
`Logger` formats lines and fans them to sinks via `CompositeSink` (Composite). Each `ILogSink` filters by its own minimum level (Strategy): `ConsoleSink` (stdout, Info) for live output and `FileSink` for a persisted copy. `Server` composes both in its constructor.

The file sink is built with `FileSink::forRun(tag)`, which writes to `server/logs/<tag>_<timestamp>.log` (tag e.g. `server_p4242`) and creates `server/logs/` best-effort. The timestamp + port keep concurrent and successive runs from clobbering each other; the path is relative to cwd, so it lands under `server/logs/` only when launched from the repo root (as the binary normally is).

## Main Loop

```cpp
int main(int argc, char** argv) {
    Server server(argc, argv);
    if (!server.shouldRun())
        return server.exitCode();
    server.run();
    return 0;
}
```

Inside `Server::run()`:

```cpp
void Server::run() {
    _clients.add(_listener.fd(), ClientType::LISTENER);
    _scheduler.schedule(resourceRespawnInterval(), [this]{ _world.spawnResources(); });

    while (!_done) {
        _clients.poll(_scheduler.msUntilNext());

        for (auto fd : _clients.newConnections())
            _onNewConnection(fd);

        for (auto& [connId, line] : _clients.readableLines())
            _dispatcher.dispatch(connId, line);

        _clients.flushWritable();
        _scheduler.tick();
    }
}
```

## Protocol Handshakes

**AI client:**
```
Server → Client: "WELCOME\n"
Client → Server: "TEAM-NAME\n"
Server → Client: "CLIENT-NUM\n"   (remaining slots for that team)
Server → Client: "X Y\n"          (map dimensions)
Client → Server: commands...
```

**GUI client:**
```
Server → Client: "WELCOME\n"
Client → Server: "GRAPHIC\n"
Server → Client: "msz X Y\n"      (map size)
              + "bct X Y ...\n"    (all tiles × width×height)
              + "tna N\n"          (all team names)
              + "sgt T\n"          (current time unit)
Client → Server: optional requests (bct, ppo, plv, pin, sgt, sst)
Server → Client: ongoing push events...
```

## Game Mechanics

### Time system
All action durations: `duration_ms = (action_cost / f) * 1000`.
- Forward/Right/Left: 7/f s
- Look/Broadcast/Eject/Take/Set: 7/f s
- Inventory: 1/f s
- Connect_nbr: immediate
- Fork: 42/f s
- Incantation: 300/f s
- Starvation: food depletes at 1 unit per 126/f s

### Resource spawning
On startup and every 20/f seconds: compute `floor(w * h * density)` per resource, distribute randomly across tiles. Minimum 1 of each type guaranteed globally.

Densities: food=0.5, linemate=0.3, deraumere=0.15, sibur=0.1, mendiane=0.1, phiras=0.08, thystame=0.05.

### Incantation
Most complex mechanic. Prerequisites checked at **start AND end** of 300/f second ritual.

Requirements per level:

| Level | Players | linemate | deraumere | sibur | mendiane | phiras | thystame |
|-------|---------|----------|-----------|-------|----------|--------|----------|
| 1→2   | 1       | 1        | 0         | 0     | 0        | 0      | 0        |
| 2→3   | 2       | 1        | 1         | 1     | 0        | 0      | 0        |
| 3→4   | 2       | 2        | 0         | 1     | 0        | 2      | 0        |
| 4→5   | 4       | 1        | 1         | 2     | 0        | 1      | 0        |
| 5→6   | 4       | 1        | 2         | 1     | 3        | 0      | 0        |
| 6→7   | 6       | 1        | 2         | 3     | 0        | 1      | 0        |
| 7→8   | 6       | 2        | 2         | 2     | 2        | 2      | 1        |

- All participants frozen during ritual (command queue suspended)
- Cross-team allowed — only level matters
- If any participant dies mid-ritual → ritual fails at end check
- Success: all participants level up, stones removed from tile

### Broadcast direction
Direction (1-8) sent to receiver is relative to receiver's orientation, based on shortest toroidal path from emitter. Same tile → direction 0.

```cpp
int broadcastDirection(int srcX, int srcY, int dstX, int dstY,
                       int mapW, int mapH, Orientation dstFacing);
```
Isolated as a pure function — easy to unit test.

### Toroidal map
Map wraps in both axes. All position arithmetic uses modulo. Player exiting right re-enters left, same for up/down.

### Fork / Egg system
1. Player sends `Fork` → queued 42/f s timer
2. On fire: egg created at player's current position, new team slot added → `enw` sent to GUI
3. New AI client connects → random egg from their team hatched → `ebo` sent → egg deleted → `edi` sent
4. `Connect_nbr` returns current available egg count for the player's team

### Ejection
`Eject` pushes all players and destroys all eggs on the tile one step in ejector's facing direction. Ejected players receive `eject: K\n` where K is direction of push.

### Win condition
First team where 6+ players reach level 8. Server sends `seg N\n` to GUI. All connections remain open until clients disconnect.

## Testing

### Against reference server
`bin/zappy_server` is the reference binary. Use it to validate our GUI behavior and as behavioral reference for our server implementation.

```bash
# Start reference server
./bin/zappy_server -p 4242 -x 10 -y 10 -n TeamA TeamB -c 5 -f 100

# Connect our GUI
./zappy_gui -p 4242

# Simulate AI with netcat
{ echo "TeamA"; sleep 0.1; echo "Forward"; sleep 0.1; echo "Look"; sleep 0.5; } | nc localhost 4242
```

### Testing our server
```bash
# Start our server
./zappy_server -p 4242 -x 10 -y 10 -n TeamA TeamB -c 5 -f 100

# Connect reference GUI (bin/zappy_gui.AppImage)
./bin/zappy_gui.AppImage -p 4242

# Multi-client stress test
for i in $(seq 1 5); do
    { echo "TeamA"; while true; do echo "Forward"; sleep 0.05; done; } | nc localhost 4242 &
done
```

### Unit test targets
- `Scheduler`: schedule, tick, msUntilNext, rescale
- `CommandDispatcher`: handshake, routing, unknown command → `ko`
- `Serializer`: all 24 protocol messages
- `AiParser` / `GuiParser`: all commands, malformed input
- `World`: resource spawn, movement, elevation table, eject logic
- `broadcastDirection`: toroidal cases, wrap-around, all 8 directions, same-tile

### E2E tests (`tests/e2e/`)
Spawn the real `zappy_server` binary and talk to it over raw TCP — no server internals linked. Each test forks the binary, connects sockets, sends wire bytes, and asserts on what comes back.

```bash
# Build and run
cmake --build build --target zappy_server zappy_server_e2e_tests
./zappy_server_e2e_tests
```

| File | What it covers |
|------|----------------|
| `test_e2e_handshake.cpp` | WELCOME on connect, two simultaneous clients, slot count + map size after team join |
| `test_e2e_ai_command.cpp` | `Forward` → `ok`, `Inventory` → `[...]`, unknown command → `ko` |
| `test_e2e_gui.cpp` | `msz` is first GUI response, `msz` command reply, `pnw` broadcast on AI connect |
| `test_e2e_incantation.cpp` | Full ritual flow: navigate to linemate tile, incantate, GUI sees `pic` + `pie 1`, player reaches level 2 |
| `test_e2e_starvation.cpp` | Starved player receives `dead`, GUI receives `pdi`, no double-death |
| `test_e2e_slot_exhaustion.cpp` | 6th client on full team gets `ko`, slot count decreases per join, other team unaffected, unknown team → `ko` |

## File Structure

```
server/
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Args.hpp/cpp
│   │   ├── Server.hpp/cpp
│   │   ├── Scheduler.hpp/cpp
│   │   ├── World.hpp/cpp
│   │   ├── Player.hpp
│   │   ├── Tile.hpp
│   │   ├── Resources.hpp
│   │   └── Orientation.hpp
│   ├── network/
│   │   ├── Listener.hpp/cpp
│   │   ├── Connection.hpp/cpp
│   │   └── ClientManager.hpp/cpp
│   ├── protocol/
│   │   ├── AiParser.hpp/cpp
│   │   ├── GuiParser.hpp/cpp
│   │   └── Serializer.hpp/cpp
│   └── game/
│       ├── CommandDispatcher.hpp/cpp
│       ├── GuiNotifier.hpp/cpp
│       └── commands/
│           ├── Forward.hpp
│           ├── Look.hpp
│           ├── Inventory.hpp
│           ├── Broadcast.hpp
│           ├── Fork.hpp
│           ├── Eject.hpp
│           ├── Take.hpp
│           ├── Set.hpp
│           └── Incantation.hpp
├── CMakeLists.txt
└── doc.md
```

## Implementation Notes

**Why `World` has no network dependency:**
Keeps game logic testable in isolation. Server layer coordinates between `World`, `Scheduler`, and `GuiNotifier`.

**Why `Scheduler` drives `poll()` timeout:**
No active waiting. When no timers are pending and no I/O arrives, the process sleeps. CPU usage near zero at idle.

**Why command queue max is 10:**
Spec requirement. Over 10 pending commands → server silently discards. Per-player, not global.

**Why `GuiNotifier` is separate from `World`:**
World is pure state. Notification is a side effect. Separation keeps both classes focused and lets us test world mutations without network mocks.

**Why one port for both client types:**
Spec mandates it. Client type is determined at handshake — team name `GRAPHIC` → GUI, any other valid team name → AI.

**Why `Serializer` is static (mirrors `ProtocolParser` in GUI):**
No state needed. Pure string generation from typed data.
