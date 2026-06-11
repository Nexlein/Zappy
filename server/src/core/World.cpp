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