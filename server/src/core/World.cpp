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
        for (auto& tile : _tiles) current += tile.resources[type];

        int deficit = target - current;
        std::uniform_int_distribution<int> dist(0, (int)_tiles.size() - 1);
        for (int j = 0; j < deficit; j++) _tiles[dist(_rng)].resources[type]++;
    }
}