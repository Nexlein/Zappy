#include "GameState.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

#include "behaviors/BroadcastBehavior.hpp"
#include "behaviors/DeathBehavior.hpp"
#include "behaviors/ForkBehavior.hpp"
#include "behaviors/LevelUpBehavior.hpp"
#include "behaviors/MoveBehavior.hpp"
#include "behaviors/TurnBehavior.hpp"
#include "renderer/raylib_helpers/RenderingHelper.hpp"

void GameState::applyEvent(const Event& e)
{
    std::visit(
        [this](const auto& event) {
            using T = std::decay_t<decltype(event)>;
            if constexpr (std::is_same_v<T, MapSize>)
                _applyMapSize(event);
            else if constexpr (std::is_same_v<T, TileContent>)
                _applyTileContent(event);
            else if constexpr (std::is_same_v<T, TeamName>)
                _applyTeamName(event);
            else if constexpr (std::is_same_v<T, PlayerNew>)
                _applyPlayerNew(event);
            else if constexpr (std::is_same_v<T, PlayerPosition>)
                _applyPlayerPosition(event);
            else if constexpr (std::is_same_v<T, PlayerLevel>)
                _applyPlayerLevel(event);
            else if constexpr (std::is_same_v<T, PlayerInventory>)
                _applyPlayerInventory(event);
            else if constexpr (std::is_same_v<T, PlayerExpulsion>)
                _applyPlayerExpulsion(event);
            else if constexpr (std::is_same_v<T, PlayerBroadcast>)
                _applyPlayerBroadcast(event);
            else if constexpr (std::is_same_v<T, IncantationStart>)
                _applyIncantationStart(event);
            else if constexpr (std::is_same_v<T, IncantationEnd>)
                _applyIncantationEnd(event);
            else if constexpr (std::is_same_v<T, PlayerFork>)
                _applyPlayerFork(event);
            else if constexpr (std::is_same_v<T, PlayerResourceDrop>)
                _applyPlayerResourceDrop(event);
            else if constexpr (std::is_same_v<T, PlayerResourceTake>)
                _applyPlayerResourceTake(event);
            else if constexpr (std::is_same_v<T, PlayerDeath>)
                _applyPlayerDeath(event);
            else if constexpr (std::is_same_v<T, EggNew>)
                _applyEggNew(event);
            else if constexpr (std::is_same_v<T, EggHatch>)
                _applyEggHatch(event);
            else if constexpr (std::is_same_v<T, EggDeath>)
                _applyEggDeath(event);
            else if constexpr (std::is_same_v<T, TimeUnit>)
                _applyTimeUnit(event);
            else if constexpr (std::is_same_v<T, TimeUnitChange>)
                _applyTimeUnitChange(event);
            else if constexpr (std::is_same_v<T, GameEnd>)
                _applyGameEnd(event);
            else if constexpr (std::is_same_v<T, ServerMessage>) {
                // Likely temporary
                std::cout << "[SERVER MESSAGE] " << event.message << std::endl;
            } else if constexpr (std::is_same_v<T, ServerUptime>)
                _applyServerUptime(event);
            else if constexpr (std::is_same_v<T, ServerSpawnedEgg>)
                _applyServerSpawnedEgg(event);
            else {
                // Is a UnknownCommand or BadParameters, we can ignore them for now
            }
        },
        e);
    dirty = true;
}

void GameState::_applyMapSize(const MapSize& e)
{
    world.width = e.width;
    world.height = e.height;
    world.tiles.resize(e.width * e.height);
}

void GameState::_applyTileContent(const TileContent& e) { world.at(e.x, e.y) = e.resources; }

void GameState::_applyTeamName(const TeamName& e) { world.teams.push_back(e.name); }

void GameState::_applyPlayerNew(const PlayerNew& e)
{
    auto [it, _] = world.players.emplace(e.id, Player{.id = e.id,
                                                      .x = e.x,
                                                      .y = e.y,
                                                      .orientation = e.orientation,
                                                      .level = e.level,
                                                      .team = e.team});
    it->second.visual.pos =
        RenderingHelper::tileToWorld(e.x, e.y, world.width, world.height, tileSize);
    it->second.visual.angle = toAngle(e.orientation);
}

void GameState::_applyPlayerPosition(const PlayerPosition& e)
{
    auto it = world.players.find(e.id);
    if (it != world.players.end()) {
        Player& player = it->second;
        int fromX = player.x;
        int fromY = player.y;
        player.x = e.x;
        player.y = e.y;
        player.orientation = e.orientation;

        float duration = timeUnit > 0 ? 7.0f / timeUnit : 0.1f;
        // remove only move/turn behaviors, preserve others (e.g. LevelUpBehavior)
        auto& behaviors = player.visual.behaviors;
        behaviors.erase(std::remove_if(behaviors.begin(), behaviors.end(),
                                       [](const auto& b) {
                                           return dynamic_cast<MoveBehavior*>(b.get()) ||
                                                  dynamic_cast<TurnBehavior*>(b.get());
                                       }),
                        behaviors.end());
        _pushBehavior(player.visual, std::make_unique<MoveBehavior>(
                                         player.visual, fromX, fromY, e.x, e.y, world.width,
                                         world.height, tileSize, duration));
        _pushBehavior(player.visual,
                      std::make_unique<TurnBehavior>(player.visual, player.visual.angle,
                                                     toAngle(e.orientation), duration));
    }
}

void GameState::_applyPlayerLevel(const PlayerLevel& e)
{
    auto it = world.players.find(e.id);
    if (it == world.players.end()) return;
    it->second.level = e.level;
    _pushBehavior(it->second.visual, std::make_unique<LevelUpBehavior>(
                                         it->second.visual, static_cast<float>(timeUnit)));
}

void GameState::_applyPlayerInventory(const PlayerInventory& e)
{
    auto it = world.players.find(e.id);
    if (it != world.players.end()) {
        it->second.inventory = e.inventory;
    }
}

void GameState::_applyPlayerExpulsion([[maybe_unused]] const PlayerExpulsion& e)
{
    // Seemingly nothing tbd for now
}

void GameState::_applyPlayerBroadcast(const PlayerBroadcast& e)
{
    auto it = world.players.find(e.id);
    if (it == world.players.end()) return;
    float mapRadius =
        std::max(static_cast<float>(world.width), static_cast<float>(world.height)) / 2.0f;
    _pushBehavior(it->second.visual,
                  std::make_unique<BroadcastBehavior>(
                      it->second.visual, static_cast<float>(timeUnit), mapRadius,
                      static_cast<float>(world.width), static_cast<float>(world.height)));
}

void GameState::_applyIncantationStart(const IncantationStart& e)
{
    for (int playerId : e.playerIds) {
        auto it = world.players.find(playerId);
        if (it != world.players.end()) {
            it->second.incanting = true;
        }
    }
}

void GameState::_applyIncantationEnd(const IncantationEnd& e)
{
    for (auto& [id, player] : world.players) {
        if (player.x == e.x && player.y == e.y && player.incanting) {
            player.incanting = false;
        }
    }
}

void GameState::_applyPlayerFork([[maybe_unused]] const PlayerFork& e)
{
    // Seemingly nothing tbd for now
}

void GameState::_applyPlayerResourceDrop(const PlayerResourceDrop& e)
{
    auto it = world.players.find(e.playerId);
    if (it != world.players.end()) {
        it->second.inventory[e.resourceId]--;
        world.at(it->second.x, it->second.y)[e.resourceId]++;
    }
}

void GameState::_applyPlayerResourceTake(const PlayerResourceTake& e)
{
    auto it = world.players.find(e.playerId);
    if (it != world.players.end()) {
        world.at(it->second.x, it->second.y)[e.resourceId]--;
        it->second.inventory[e.resourceId]++;
    }
}

void GameState::_applyPlayerDeath(const PlayerDeath& e)
{
    auto it = world.players.find(e.id);
    if (it == world.players.end()) return;
    Player dying = std::move(it->second);
    world.players.erase(it);
    dying.visual.behaviors.clear();
    world.dyingPlayers[e.id] = std::move(dying);
    Player& settled = world.dyingPlayers[e.id];
    _pushBehavior(settled.visual,
                  std::make_unique<DeathBehavior>(settled.visual, static_cast<float>(timeUnit)));
}

void GameState::_applyEggNew(const EggNew& e)
{
    auto it = world.players.find(e.playerId);
    if (it == world.players.end()) {
        return;
    }
    Egg egg{.id = e.eggId, .x = e.x, .y = e.y, .team = it->second.team};
    egg.visual.pos = RenderingHelper::tileToWorld(e.x, e.y, world.width, world.height, tileSize);
    world.eggs[e.eggId] = std::move(egg);
    Egg& settled = world.eggs[e.eggId];
    _pushBehavior(settled.visual,
                  std::make_unique<ForkBehavior>(settled.visual, static_cast<float>(timeUnit)));
}

void GameState::_applyEggHatch(const EggHatch& e)
{
    world.eggs.erase(e.id);
    // Spawning handled by PlayerNew event, so nothing else to do here
}

void GameState::_applyEggDeath(const EggDeath& e) { world.eggs.erase(e.id); }

void GameState::_applyTimeUnit(const TimeUnit& e) { timeUnit = e.timeUnit; }

void GameState::_applyTimeUnitChange(const TimeUnitChange& e) { timeUnit = e.timeUnit; }

void GameState::_applyGameEnd(const GameEnd& e) { winnerTeam = e.winningTeam; }

void GameState::_applyServerUptime(const ServerUptime& e) { serverUptimeSeconds = e.uptimeSeconds; }

void GameState::_applyServerSpawnedEgg(const ServerSpawnedEgg& e)
{
    std::cout << "Server spawned egg: id=" << e.eggId << ", team=" << e.team
              << ", x=" << e.x << ", y=" << e.y << std::endl;
    Egg egg{.id = e.eggId, .x = e.x, .y = e.y, .team = e.team};
    egg.visual.pos = RenderingHelper::tileToWorld(e.x, e.y, world.width, world.height, tileSize);
    world.eggs[e.eggId] = std::move(egg);
    Egg& settled = world.eggs[e.eggId];
    _pushBehavior(settled.visual,
                  std::make_unique<ForkBehavior>(settled.visual, static_cast<float>(timeUnit)));
}

void GameState::_pushBehavior(VisualState& visual, std::unique_ptr<IBehavior> b)
{
    if (b->getDuration() >= b->minDuration()) visual.behaviors.push_back(std::move(b));
}
