#pragma once

#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "data/Egg.hpp"
#include "data/Player.hpp"
#include "data/Resources.hpp"
#include "data/Tile.hpp"

class World {
public:
    World(int width, int height, const std::vector<std::string>& teamNames, int clientNb);

    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    void spawnResources();

private:
    int _width;
    int _height;
    std::vector<Tile> _tiles;
    std::unordered_map<int, Player> _players;
    std::unordered_map<int, Egg> _eggs;
    std::vector<std::string> _teamNames;
    int _clientNb;
    std::mt19937 _rng{std::random_device{}()};
};