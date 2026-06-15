#include "World.hpp"

#include <algorithm>

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

void World::spawnResources()
{
    for (int i = 0; i < Resources::TYPE_COUNT; i++) {
        auto type = static_cast<ResourceType>(i);

        int target = std::max(1, (int)(_width * _height * Resources::density(type)));

        int current = 0;
        for (auto& tile : _tiles) current += tile.resources[type];

        int deficit = target - current;
        std::uniform_int_distribution<int> dist(0, (int)_tiles.size() - 1);
        for (int j = 0; j < deficit; j++) _tiles[dist(_rng)].resources[type]++;
    }
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
    _players[id] = p;
    at(x, y).playerIds.push_back(id);
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
}

void World::movePlayer(int id, int x, int y)
{
    auto& p = _players.at(id);
    auto& oldIds = at(p.x, p.y).playerIds;
    oldIds.erase(std::remove(oldIds.begin(), oldIds.end(), id), oldIds.end());
    p.x = ((x % _width) + _width) % _width;
    p.y = ((y % _height) + _height) % _height;
    at(p.x, p.y).playerIds.push_back(id);
}

bool World::takeResource(int playerId, ResourceType type)
{
    auto& p = _players.at(playerId);
    auto& tile = at(p.x, p.y);
    if (tile.resources[type] <= 0) return false;
    tile.resources[type]--;
    p.inventory[type]++;
    return true;
}

bool World::setResource(int playerId, ResourceType type)
{
    auto& p = _players.at(playerId);
    if (p.inventory[type] <= 0) return false;
    p.inventory[type]--;
    at(p.x, p.y).resources[type]++;
    return true;
}

Player& World::getPlayer(int id) { return _players.at(id); }

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

    for (int eid : tile.eggIds) _eggs.erase(eid);
    tile.eggIds.clear();

    return {toEject, dx, dy};
}

int World::addEgg(int playerId)
{
    auto& p = _players.at(playerId);
    int eid = _nextEggId++;
    Egg egg{eid, p.x, p.y, p.teamName};
    _eggs[eid] = egg;
    at(p.x, p.y).eggIds.push_back(eid);
    return eid;
}

bool World::hatchEgg(int eggId)
{
    auto it = _eggs.find(eggId);
    if (it == _eggs.end()) return false;
    auto& egg = it->second;
    auto& ids = at(egg.x, egg.y).eggIds;
    ids.erase(std::remove(ids.begin(), ids.end(), eggId), ids.end());
    _eggs.erase(it);
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

    return participants;
}

bool World::finalizeIncantation(const std::vector<int>& participantIds)
{
    if (participantIds.empty()) return false;

    auto it = _players.find(participantIds[0]);
    if (it == _players.end()) {
        for (int pid : participantIds) {
            auto p = _players.find(pid);
            if (p != _players.end()) p->second.isIncanting = false;
        }
        return false;
    }

    int level = it->second.level;
    auto& tile = at(it->second.x, it->second.y);

    if (!_checkReqs(tile, participantIds, level, _players)) {
        for (int pid : participantIds) {
            auto p = _players.find(pid);
            if (p != _players.end()) p->second.isIncanting = false;
        }
        return false;
    }

    const auto& req = INCANTATION_REQS[level - 1];
    tile.resources[ResourceType::LINEMATE] -= req.linemate;
    tile.resources[ResourceType::DERAUMERE] -= req.deraumere;
    tile.resources[ResourceType::SIBUR] -= req.sibur;
    tile.resources[ResourceType::MENDIANE] -= req.mendiane;
    tile.resources[ResourceType::PHIRAS] -= req.phiras;
    tile.resources[ResourceType::THYSTAME] -= req.thystame;

    for (int pid : participantIds) {
        auto p = _players.find(pid);
        if (p != _players.end()) {
            p->second.level += 1;
            p->second.isIncanting = false;
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

std::optional<std::string> World::checkWin() const
{
    std::unordered_map<std::string, int> level8count;
    for (auto& [id, p] : _players)
        if (p.level == 8) level8count[p.teamName]++;

    for (auto& [team, count] : level8count)
        if (count >= 6) return team;

    return std::nullopt;
}
