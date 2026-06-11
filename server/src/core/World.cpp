#include "World.hpp"

#include <algorithm>

World::World(int width, int height, const std::vector<std::string>& teamNames, int clientNb)
    : _width(width), _height(height), _teamNames(teamNames), _clientNb(clientNb)
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
        for (auto& tile : _tiles)
            current += tile.resources[type];

        int deficit = target - current;
        std::uniform_int_distribution<int> dist(0, (int)_tiles.size() - 1);
        for (int j = 0; j < deficit; j++)
            _tiles[dist(_rng)].resources[type]++;
    }
}

int World::addPlayer(int connectionId, const std::string& teamName, int x, int y, Orientation orientation)
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
    if (tile.resources[type] <= 0)
        return false;
    tile.resources[type]--;
    p.inventory[type]++;
    return true;
}

bool World::setResource(int playerId, ResourceType type)
{
    auto& p = _players.at(playerId);
    if (p.inventory[type] <= 0)
        return false;
    p.inventory[type]--;
    at(p.x, p.y).resources[type]++;
    return true;
}

Player& World::getPlayer(int id)
{
    return _players.at(id);
}

EjectResult World::ejectPlayers(int ejectorId)
{
    auto& ejector = _players.at(ejectorId);

    int dx = 0, dy = 0;
    switch (ejector.orientation) {
        case Orientation::N: dy = -1; break;
        case Orientation::S: dy =  1; break;
        case Orientation::E: dx =  1; break;
        case Orientation::W: dx = -1; break;
    }

    auto& tile = at(ejector.x, ejector.y);

    std::vector<int> toEject;
    for (int pid : tile.playerIds)
        if (pid != ejectorId)
            toEject.push_back(pid);

    for (int pid : toEject)
        movePlayer(pid, _players.at(pid).x + dx, _players.at(pid).y + dy);

    for (int eid : tile.eggIds)
        _eggs.erase(eid);
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
    if (it == _eggs.end())
        return false;
    auto& egg = it->second;
    auto& ids = at(egg.x, egg.y).eggIds;
    ids.erase(std::remove(ids.begin(), ids.end(), eggId), ids.end());
    _eggs.erase(it);
    return true;
}
