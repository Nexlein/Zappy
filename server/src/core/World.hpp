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
#include "data/Orientation.hpp"

struct EjectResult {
    std::vector<int> ejectedPlayerIds;
    int dx;
    int dy;
};

class World {
public:
    World(int width, int height, const std::vector<std::string>& teamNames, int clientNb);

    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    void spawnResources();

    int addPlayer(int connectionId, const std::string& teamName, int x, int y, Orientation orientation);
    void removePlayer(int id);
    void movePlayer(int id, int x, int y);

    bool takeResource(int playerId, ResourceType type);
    bool setResource(int playerId, ResourceType type);

    Player& getPlayer(int id);

    EjectResult ejectPlayers(int ejectorId);

    int addEgg(int playerId);
    bool hatchEgg(int eggId);

private:
    int _width;
    int _height;
    std::vector<Tile> _tiles;
    std::unordered_map<int, Player> _players;
    std::unordered_map<int, Egg> _eggs;
    std::vector<std::string> _teamNames;
    int _clientNb;
    std::mt19937 _rng{std::random_device{}()};
    int _nextPlayerId = 0;
    int _nextEggId = 0;
};