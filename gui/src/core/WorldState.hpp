#pragma once

#include "Resources.hpp"
#include "Orientation.hpp"
#include <vector>
#include <unordered_map>
#include <string>

struct Player {
    int id;
    int x;
    int y;
    Orientation orientation;
    int level;
    std::string team;
    Resources inventory;
    int timeToLive;
    bool incanting;
};

struct Egg {
    int id;
    int x;
    int y;
    std::string team;
};

class WorldState {
public:
    int width;
    int height;
    std::vector<Resources> tiles;
    std::unordered_map<int, Player> players;
    std::unordered_map<int, Egg> eggs;
    std::vector<std::string> teams;
    std::string winnerTeam;

    Resources& at(int x, int y) {
        return tiles[y * width + x];
    }

    const Resources& at(int x, int y) const {
        return tiles[y * width + x];
    }
};
