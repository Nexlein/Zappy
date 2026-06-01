#include "GameState.hpp"
#include <iostream>

void GameState::applyEvent(const Event& e) {
    std::visit([this](const auto& event) {
        using T = std::decay_t<decltype(event)>;
        if      constexpr (std::is_same_v<T, MapSize>)              applyMapSize(event);
        else if constexpr (std::is_same_v<T, TileContent>)          applyTileContent(event);
        else if constexpr (std::is_same_v<T, TeamName>)             applyTeamName(event);
        else if constexpr (std::is_same_v<T, PlayerNew>)            applyPlayerNew(event);
        else if constexpr (std::is_same_v<T, PlayerPosition>)       applyPlayerPosition(event);
        else if constexpr (std::is_same_v<T, PlayerLevel>)          applyPlayerLevel(event);
        else if constexpr (std::is_same_v<T, PlayerInventory>)      applyPlayerInventory(event);
        else if constexpr (std::is_same_v<T, PlayerExpulsion>)      applyPlayerExpulsion(event);
        else if constexpr (std::is_same_v<T, PlayerBroadcast>)      applyPlayerBroadcast(event);
        else if constexpr (std::is_same_v<T, IncantationStart>)     applyIncantationStart(event);
        else if constexpr (std::is_same_v<T, IncantationEnd>)       applyIncantationEnd(event);
        else if constexpr (std::is_same_v<T, PlayerFork>)           applyPlayerFork(event);
        else if constexpr (std::is_same_v<T, PlayerResourceDrop>)   applyPlayerResourceDrop(event);
        else if constexpr (std::is_same_v<T, PlayerResourceTake>)   applyPlayerResourceTake(event);
        else if constexpr (std::is_same_v<T, PlayerDeath>)          applyPlayerDeath(event);
        else if constexpr (std::is_same_v<T, EggNew>)               applyEggNew(event);
        else if constexpr (std::is_same_v<T, EggHatch>)             applyEggHatch(event);
        else if constexpr (std::is_same_v<T, EggDeath>)             applyEggDeath(event);
        else if constexpr (std::is_same_v<T, TimeUnit>)             applyTimeUnit(event);
        else if constexpr (std::is_same_v<T, TimeUnitChange>)       applyTimeUnitChange(event);
        else if constexpr (std::is_same_v<T, GameEnd>)              applyGameEnd(event);
        else if constexpr (std::is_same_v<T, ServerMessage>) {
            // Likely temporary
            std::cout << "[SERVER MESSAGE] " << event.message << std::endl;
        } else {
            // Is a UnknownCommand or BadParameters, we can ignore them for now
        }
    }, e);
    dirty = true;
}

void GameState::applyMapSize(const MapSize& e) {
    world.width = e.width;
    world.height = e.height;
    world.tiles.resize(e.width * e.height);
}

void GameState::applyTileContent(const TileContent& e) {
    world.at(e.x, e.y) = e.resources;
}

void GameState::applyTeamName(const TeamName& e) {
    world.teams.push_back(e.name);
}

void GameState::applyPlayerNew(const PlayerNew& e) {
    Player player{
        .id = e.id,
        .x = e.x,
        .y = e.y,
        .orientation = e.orientation,
        .level = e.level,
        .team = e.team
    };
    world.players[e.id] = player;
}

void GameState::applyPlayerPosition(const PlayerPosition& e) {
    auto it = world.players.find(e.id);
    if (it != world.players.end()) {
        it->second.x = e.x;
        it->second.y = e.y;
        it->second.orientation = e.orientation;
    }
}

void GameState::applyPlayerLevel(const PlayerLevel& e) {
    auto it = world.players.find(e.id);
    if (it != world.players.end()) {
        it->second.level = e.level;
    }
}

void GameState::applyPlayerInventory(const PlayerInventory& e) {
    auto it = world.players.find(e.id);
    if (it != world.players.end()) {
        it->second.inventory = e.inventory;
    }
}

void GameState::applyPlayerExpulsion([[maybe_unused]] const PlayerExpulsion& e) {
    // Seemingly nothing tbd for now
}

void GameState::applyPlayerBroadcast([[maybe_unused]] const PlayerBroadcast& e) {
    // Seemingly nothing tbd for now
}

void GameState::applyIncantationStart(const IncantationStart& e) {
    for (int playerId : e.playerIds) {
        auto it = world.players.find(playerId);
        if (it != world.players.end()) {
            it->second.incanting = true;
        }
    }
}

void GameState::applyIncantationEnd(const IncantationEnd& e) {
    for (auto& [id, player] : world.players) {
        if (player.x == e.x && player.y == e.y && player.incanting) {
            player.incanting = false;
        }
    }
}

void GameState::applyPlayerFork([[maybe_unused]] const PlayerFork& e) {
    // Seemingly nothing tbd for now
}

void GameState::applyPlayerResourceDrop(const PlayerResourceDrop& e) {
    auto it = world.players.find(e.playerId);
    if (it != world.players.end()) {
        it->second.inventory[e.resourceId]--;
        world.at(it->second.x, it->second.y)[e.resourceId]++;
    }
}

void GameState::applyPlayerResourceTake(const PlayerResourceTake& e) {
    auto it = world.players.find(e.playerId);
    if (it != world.players.end()) {
        world.at(it->second.x, it->second.y)[e.resourceId]--;
        it->second.inventory[e.resourceId]++;
    }
}

void GameState::applyPlayerDeath(const PlayerDeath& e) {
    world.players.erase(e.id);
}

void GameState::applyEggNew(const EggNew& e) {
    auto it = world.players.find(e.playerId);
    if (it == world.players.end()) {
        return;
    }
    Egg egg{
        .id = e.eggId,
        .x = e.x,
        .y = e.y,
        .team = it->second.team
    };
    world.eggs[e.eggId] = egg;
}

void GameState::applyEggHatch(const EggHatch& e) {
    world.eggs.erase(e.id);
    // Spawning handled by PlayerNew event, so nothing else to do here
}

void GameState::applyEggDeath(const EggDeath& e) {
    world.eggs.erase(e.id);
}

void GameState::applyTimeUnit(const TimeUnit& e) {
    timeUnit = e.timeUnit;
}

void GameState::applyTimeUnitChange(const TimeUnitChange& e) {
    timeUnit = e.timeUnit;
}

void GameState::applyGameEnd(const GameEnd& e) {
    winnerTeam = e.winningTeam;
}
