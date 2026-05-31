#pragma once

#include "Resources.hpp"
#include "Orientation.hpp"
#include <vector>
#include <string>
#include <ostream>
#include <unordered_map>

struct Player {
    int id;
    int x;
    int y;
    Orientation orientation;
    int level;
    std::string team;
    Resources inventory = {};
    bool incanting = false;
};

inline std::ostream& operator<<(std::ostream& os, const Player& player) {
    os << "Player " << player.id << ":\n"
       << "  From team " << player.team << ", level " << player.level << "\n"
       << "  At " << player.x << ", " << player.y << ", facing " << player.orientation << "\n"
       << "  Inventory: " << player.inventory << "\n"
       << "  Incanting: " << (player.incanting ? "yes" : "no");
    return os;
}

struct Egg {
    int id;
    int x;
    int y;
    std::string team;
};

inline std::ostream& operator<<(std::ostream& os, const Egg& egg) {
    os << "Egg " << egg.id << ":\n"
       << "  From team " << egg.team << "\n"
       << "  At " << egg.x << ", " << egg.y;
    return os;
}

class WorldState {
public:
    int width;
    int height;
    std::vector<Resources> tiles;
    std::unordered_map<int, Player> players;
    std::unordered_map<int, Egg> eggs;
    std::vector<std::string> teams;

    Resources& at(int x, int y) {
        return tiles[y * width + x];
    }

    const Resources& at(int x, int y) const {
        return tiles[y * width + x];
    }
};
