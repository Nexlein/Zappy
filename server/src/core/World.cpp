#include "World.hpp"

#include <algorithm>
#include <set>

World::World(int width, int height, const std::vector<std::string>& teamNames)
    : _width(width), _height(height), _teamNames(teamNames)
{
    _tiles = std::vector<Tile>(width * height);
}

Tile& World::at(int x, int y)
{
    int nx = ((x % _width) + _width) % _width;
    int ny = ((y % _height) + _height) % _height;

    return _tiles[ny * _width + nx];
}

const Tile& World::at(int x, int y) const
{
    int nx = ((x % _width) + _width) % _width;
    int ny = ((y % _height) + _height) % _height;

    return _tiles[ny * _width + nx];
}

std::vector<std::pair<int, int>> World::spawnResources()
{
    std::set<int> changedIndices;

    for (int i = 0; i < Resources::TYPE_COUNT; i++) {
        auto type = static_cast<ResourceType>(i);

        int target = std::max(1, (int)(_width * _height * Resources::density(type)));

        int current = 0;
        for (auto& tile : _tiles) current += tile.resources[type];

        int deficit = target - current;
        std::uniform_int_distribution<int> dist(0, (int)_tiles.size() - 1);
        for (int j = 0; j < deficit; j++) {
            int idx = dist(_rng);
            _tiles[idx].resources[type]++;
            changedIndices.insert(idx);
        }
    }

    std::vector<std::pair<int, int>> changed;

    changed.reserve(changedIndices.size());
    for (int idx : changedIndices) changed.emplace_back(idx % _width, idx / _width);
    for (auto* observer : _observers)
        for (auto [x, y] : changed) observer->onTileChanged(x, y, at(x, y).resources);
    return changed;
}

int World::addPlayer(int connectionId, const std::string& teamName, int x, int y,
                     Orientation orientation)
{
    int id = _nextPlayerId++;
    Player p;

    p.id = id;
    p.connectionId = connectionId;
    p.teamName = teamName;
    p.x = x;
    p.y = y;
    p.orientation = orientation;
    p.inventory.food = 10;
    _players[id] = p;
    at(x, y).playerIds.push_back(id);
    for (auto* observer : _observers)
        observer->onPlayerAdded(id, x, y, orientation, p.level, teamName);
    return id;
}

void World::removePlayer(int id)
{
    auto it = _players.find(id);
    if (it == _players.end()) return;

    auto& p = it->second;
    auto& ids = at(p.x, p.y).playerIds;

    ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
    _players.erase(it);
    for (auto* observer : _observers) observer->onPlayerRemoved(id);
}

void World::movePlayer(int id, int x, int y)
{
    auto& p = _players.at(id);
    auto& oldIds = at(p.x, p.y).playerIds;

    oldIds.erase(std::remove(oldIds.begin(), oldIds.end(), id), oldIds.end());
    p.x = ((x % _width) + _width) % _width;
    p.y = ((y % _height) + _height) % _height;
    at(p.x, p.y).playerIds.push_back(id);
    for (auto* observer : _observers) observer->onPlayerMoved(id, p.x, p.y, p.orientation);
}

void World::turnPlayer(int id, Orientation orientation)
{
    auto& p = _players.at(id);
    p.orientation = orientation;
    for (auto* observer : _observers) observer->onPlayerMoved(id, p.x, p.y, p.orientation);
}

bool World::takeResource(int playerId, ResourceType type)
{
    auto& p = _players.at(playerId);
    auto& tile = at(p.x, p.y);

    if (tile.resources[type] <= 0) return false;
    tile.resources[type]--;
    p.inventory[type]++;
    for (auto* observer : _observers)
        observer->onResourceTaken(playerId, type, p.x, p.y, tile.resources);
    return true;
}

bool World::setResource(int playerId, ResourceType type)
{
    auto& p = _players.at(playerId);

    if (p.inventory[type] <= 0) return false;
    p.inventory[type]--;
    at(p.x, p.y).resources[type]++;
    for (auto* observer : _observers)
        observer->onResourceDropped(playerId, type, p.x, p.y, at(p.x, p.y).resources);
    return true;
}

bool World::consumeFood(int playerId)
{
    auto it = _players.find(playerId);
    if (it == _players.end()) return false;

    auto& p = it->second;
    p.inventory.food--;
    for (auto* observer : _observers)
        observer->onPlayerInventoryChanged(playerId, p.x, p.y, p.inventory);
    return p.inventory.food > 0;
}

Player& World::getPlayer(int id) { return _players.at(id); }

void World::playerBroadcast(int playerId, const std::string& message)
{
    for (auto* observer : _observers) observer->onBroadcast(playerId, message);
}

EjectResult World::ejectPlayers(int ejectorId)
{
    auto& ejector = _players.at(ejectorId);

    int dx = 0, dy = 0;
    switch (ejector.orientation) {
        case Orientation::N:
            dy = -1;
            break;
        case Orientation::S:
            dy = 1;
            break;
        case Orientation::E:
            dx = 1;
            break;
        case Orientation::W:
            dx = -1;
            break;
    }

    auto& tile = at(ejector.x, ejector.y);

    std::vector<int> toEject;
    for (int pid : tile.playerIds)
        if (pid != ejectorId) toEject.push_back(pid);

    for (int pid : toEject) movePlayer(pid, _players.at(pid).x + dx, _players.at(pid).y + dy);

    for (int eid : tile.eggIds) {
        _eggs.erase(eid);
        for (auto* observer : _observers) observer->onEggDied(eid);
    }
    tile.eggIds.clear();

    for (int pid : toEject) {
        for (auto* observer : _observers) observer->onPlayerEjected(pid);
    }
    return {toEject, dx, dy};
}

int World::_spawnEgg(const std::string& teamName, int x, int y, int parentPlayerId)
{
    int eid = _nextEggId++;

    Egg egg{eid, parentPlayerId, x, y, teamName};
    _eggs[eid] = egg;
    at(x, y).eggIds.push_back(eid);
    if (parentPlayerId < 0)
        for (auto* observer : _observers) observer->onInitialEggSpawned(eid, teamName, x, y);
    else
        for (auto* observer : _observers) observer->onEggLaid(eid, parentPlayerId, x, y);
    return eid;
}

void World::spawnInitialEggs(int countPerTeam)
{
    std::uniform_int_distribution<int> dx(0, _width - 1);
    std::uniform_int_distribution<int> dy(0, _height - 1);
    for (const auto& team : _teamNames)
        for (int i = 0; i < countPerTeam; i++) _spawnEgg(team, dx(_rng), dy(_rng), -1);
}

int World::addEgg(int playerId)
{
    auto& p = _players.at(playerId);
    return _spawnEgg(p.teamName, p.x, p.y, p.id);
}

int World::teamEggCount(const std::string& team) const
{
    int count = 0;
    for (const auto& [id, egg] : _eggs)
        if (egg.teamName == team) count++;
    return count;
}

bool World::hatchEgg(int eggId)
{
    auto it = _eggs.find(eggId);
    if (it == _eggs.end()) return false;

    auto& egg = it->second;
    auto& ids = at(egg.x, egg.y).eggIds;

    ids.erase(std::remove(ids.begin(), ids.end(), eggId), ids.end());
    _eggs.erase(it);
    for (auto* observer : _observers) observer->onEggHatched(eggId);
    return true;
}

std::optional<Egg> World::popEggForTeam(const std::string& teamName)
{
    for (auto it = _eggs.begin(); it != _eggs.end(); ++it) {
        if (it->second.teamName == teamName) {
            Egg egg = it->second;
            auto& ids = at(egg.x, egg.y).eggIds;
            ids.erase(std::remove(ids.begin(), ids.end(), egg.id), ids.end());
            _eggs.erase(it);
            for (auto* observer : _observers) observer->onEggHatched(egg.id);
            return egg;
        }
    }
    return std::nullopt;
}

static const IncantationReq INCANTATION_REQS[7] = {
    {1, 1, 0, 0, 0, 0, 0},  // lvl 1 -> lvl 2
    {2, 1, 1, 1, 0, 0, 0},  // lvl 2 -> lvl 3
    {2, 2, 0, 1, 0, 2, 0},  // lvl 3 -> lvl 4
    {4, 1, 1, 2, 0, 1, 0},  // lvl 4 -> lvl 5
    {4, 1, 2, 1, 3, 0, 0},  // lvl 5 -> lvl 6
    {6, 1, 2, 3, 0, 1, 0},  // lvl 6 -> lvl 7
    {6, 2, 2, 2, 2, 2, 1},  // lvl 7 -> lvl 8
};

static bool _checkReqs(const Tile& tile, const std::vector<int>& participants, int level,
                       const std::unordered_map<int, Player>& players)
{
    const auto& req = INCANTATION_REQS[level - 1];

    // count participants still on tile at correct level
    int count = 0;
    for (int pid : participants) {
        auto it = players.find(pid);
        if (it == players.end()) return false;

        const auto& p = it->second;
        if (p.level != level) return false;

        bool onTile = false;
        for (int tid : tile.playerIds)
            if (tid == pid) {
                onTile = true;
                break;
            }
        if (!onTile) return false;
        count++;
    }
    if (count < req.playerCount) return false;

    if (tile.resources[ResourceType::LINEMATE] < req.linemate) return false;
    if (tile.resources[ResourceType::DERAUMERE] < req.deraumere) return false;
    if (tile.resources[ResourceType::SIBUR] < req.sibur) return false;
    if (tile.resources[ResourceType::MENDIANE] < req.mendiane) return false;
    if (tile.resources[ResourceType::PHIRAS] < req.phiras) return false;
    if (tile.resources[ResourceType::THYSTAME] < req.thystame) return false;

    return true;
}

std::optional<std::vector<int>> World::startIncantation(int playerId)
{
    auto& initiator = _players.at(playerId);
    int level = initiator.level;
    if (level < 1 || level > 7) return std::nullopt;

    auto& tile = at(initiator.x, initiator.y);
    const auto& req = INCANTATION_REQS[level - 1];

    std::vector<int> participants;
    for (int pid : tile.playerIds) {
        auto& p = _players.at(pid);
        if (p.level == level) participants.push_back(pid);
    }

    if (static_cast<int>(participants.size()) < req.playerCount) return std::nullopt;

    if (!_checkReqs(tile, participants, level, _players)) return std::nullopt;

    for (int pid : participants) _players.at(pid).isIncanting = true;

    for (auto* observer : _observers)
        observer->onIncantationStart(initiator.x, initiator.y, initiator.level, participants);

    return participants;
}

bool World::finalizeIncantation(int x, int y, const std::vector<int>& participantIds)
{
    if (participantIds.empty()) return false;

    auto it = _players.find(participantIds[0]);
    if (it == _players.end()) {
        for (int pid : participantIds) {
            auto p = _players.find(pid);
            if (p != _players.end()) p->second.isIncanting = false;
        }
        for (auto* obs : _observers) obs->onIncantationEnd(x, y, false);
        return false;
    }

    int level = it->second.level;
    auto& tile = at(it->second.x, it->second.y);

    if (!_checkReqs(tile, participantIds, level, _players)) {
        for (int pid : participantIds) {
            auto p = _players.find(pid);
            if (p != _players.end()) p->second.isIncanting = false;
        }
        for (auto* obs : _observers) obs->onIncantationEnd(x, y, false);
        return false;
    }

    const auto& req = INCANTATION_REQS[level - 1];
    tile.resources[ResourceType::LINEMATE] -= req.linemate;
    tile.resources[ResourceType::DERAUMERE] -= req.deraumere;
    tile.resources[ResourceType::SIBUR] -= req.sibur;
    tile.resources[ResourceType::MENDIANE] -= req.mendiane;
    tile.resources[ResourceType::PHIRAS] -= req.phiras;
    tile.resources[ResourceType::THYSTAME] -= req.thystame;

    int newLevel = level + 1;
    for (int pid : participantIds) {
        auto p = _players.find(pid);
        if (p != _players.end()) {
            p->second.level = newLevel;
            p->second.isIncanting = false;
        }
    }

    for (auto* obs : _observers) obs->onIncantationEnd(x, y, true);
    for (int pid : participantIds) {
        if (_players.count(pid))
            for (auto* obs : _observers) obs->onPlayerLevelUp(pid, newLevel);
    }

    if (!_gameEnded) {
        auto winner = checkWin();
        if (winner) {
            _gameEnded = true;
            for (auto* obs : _observers) obs->onGameEnd(*winner);
        }
    }

    return true;
}

int World::teamPlayerCount(const std::string& team) const
{
    int count = 0;
    for (auto& [id, p] : _players)
        if (p.teamName == team) count++;
    return count;
}

int World::width() const { return _width; }
int World::height() const { return _height; }
const std::unordered_map<int, Player>& World::getPlayers() const { return _players; }
const std::unordered_map<int, Egg>& World::getEggs() const { return _eggs; }

std::optional<std::string> World::checkWin() const
{
    std::unordered_map<std::string, int> level8count;
    for (auto& [id, p] : _players)
        if (p.level == 8) level8count[p.teamName]++;

    for (auto& [team, count] : level8count)
        if (count >= 6) return team;

    return std::nullopt;
}

void World::addWorldObserver(IWorldObserver* observer) { _observers.push_back(observer); }
