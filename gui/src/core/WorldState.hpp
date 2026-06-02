#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Orientation.hpp"
#include "Resources.hpp"

/**
 * @brief Represents a player in the game.
 */
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

inline std::ostream& operator<<(std::ostream& os, const Player& player)
{
    os << "Player " << player.id << ":\n"
       << "  From team " << player.team << ", level " << player.level << "\n"
       << "  At " << player.x << ", " << player.y << ", facing " << player.orientation << "\n"
       << "  Inventory: " << player.inventory << "\n"
       << "  Incanting: " << (player.incanting ? "yes" : "no");
    return os;
}

/**
 * @brief Represents an egg in the game.
 */
struct Egg {
    int id;
    int x;
    int y;
    std::string team;
};

inline std::ostream& operator<<(std::ostream& os, const Egg& egg)
{
    os << "Egg " << egg.id << ":\n"
       << "  From team " << egg.team << "\n"
       << "  At " << egg.x << ", " << egg.y;
    return os;
}

/**
 * @brief Represents the state of the world in the game, including the map size, tile contents,
 * players, eggs, and teams.
 */
class WorldState {
    public:
    int width;
    int height;
    std::vector<Resources> tiles;
    std::unordered_map<int, Player> players;
    std::unordered_map<int, Egg> eggs;
    std::vector<std::string> teams;

    /**
     * @brief Accesses the resources at a specific tile coordinate (x, y).
     */
    Resources& at(int x, int y) { return tiles[y * width + x]; }

    /**
     * @brief Accesses the resources at a specific tile coordinate (x, y).
     */
    const Resources& at(int x, int y) const { return tiles[y * width + x]; }
};
