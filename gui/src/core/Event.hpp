#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "Orientation.hpp"
#include "Resources.hpp"

// See doc for all Events at @file:G-YEP-400_zappy_GUI_protocol.pdf

// msz X Y
struct MapSize {
    int width;
    int height;
};

// bct X Y q0 q1 q2 q3 q4 q5 q6
struct TileContent {
    int x;
    int y;
    Resources resources;
};

// tna N
struct TeamName {
    std::string name;
};

// pnw #n X Y O L N
struct PlayerNew {
    int id;
    int x;
    int y;
    Orientation orientation;
    int level;
    std::string team;
};

// ppo #n X Y O
struct PlayerPosition {
    int id;
    int x;
    int y;
    Orientation orientation;
};

// plv #n L
struct PlayerLevel {
    int id;
    int level;
};

// pin #n X Y q0 q1 q2 q3 q4 q5 q6
struct PlayerInventory {
    int id;
    int x;
    int y;
    Resources inventory;
};

// pex #n
struct PlayerExpulsion {
    int id;
};

// pbc #n M
struct PlayerBroadcast {
    int id;
    std::string message;
};

// pic X Y L #n #n ...
struct IncantationStart {
    int x;
    int y;
    int level;
    std::vector<int> playerIds;
};

// pie X Y R
struct IncantationEnd {
    int x;
    int y;
    bool success;
};

// pfk #n
struct PlayerFork {
    int id;
};

// pdr #n i
struct PlayerResourceDrop {
    int playerId;
    int resourceId;
};

// pgt #n i
struct PlayerResourceTake {
    int playerId;
    int resourceId;
};

// pdi #n
struct PlayerDeath {
    int id;
};

// enw #e #n X Y
struct EggNew {
    int eggId;
    int playerId;
    int x;
    int y;
};

// ebo #e
struct EggHatch {
    int id;
};

// edi #e
struct EggDeath {
    int id;
};

// sgt T
struct TimeUnit {
    int timeUnit;
};

// sst T
struct TimeUnitChange {
    int timeUnit;
};

// seg N
struct GameEnd {
    std::string winningTeam;
};

// gtt <team> <seconds> <ticks>
struct TeamJoinTime {
    std::string team;
    int seconds;    // -1 if the team never joined
    int64_t ticks;  // game-time ticks at first join (-1 if never joined)
};

// smg M
struct ServerMessage {
    std::string message;
};

// suc
struct UnknownCommand {};

// sbp
struct BadParameters {};

// stu x x (this is a custom event, not part of the official protocol, representing the server
// uptime in seconds, and the number of ticks since the server started)
struct ServerUptime {
    int uptimeSeconds;
    int tickCount;
};

// sse #e N X Y (this is a custom event, not part of the official protocol, representing an egg
// spawned by the server)
struct ServerSpawnedEgg {
    int eggId;
    std::string team;
    int x;
    int y;
};

/**
 * @brief Represents an event received from the server.
 * Is a variant of all possible events, each with their own parameters.
 */
using Event =
    std::variant<MapSize, TileContent, TeamName, PlayerNew, PlayerPosition, PlayerLevel,
                 PlayerInventory, PlayerExpulsion, PlayerBroadcast, IncantationStart,
                 IncantationEnd, PlayerFork, PlayerResourceDrop, PlayerResourceTake, PlayerDeath,
                 EggNew, EggHatch, EggDeath, TimeUnit, TimeUnitChange, GameEnd, ServerMessage,
                 UnknownCommand, BadParameters, ServerUptime, ServerSpawnedEgg, TeamJoinTime>;
