#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Orientation.hpp"
#include "Resources.hpp"
#include "VisualState.hpp"

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
    VisualState visual = {};
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
    float rotation = static_cast<float>(rand() % 360);
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
    mutable std::unordered_map<int, Player> dyingPlayers;
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

    /**
     * @brief Checks if a player with the given ID exists in the world.
     */
    bool playerExists(int id) const { return players.find(id) != players.end(); }

    /**
     * @brief Checks if an egg with the given ID exists in the world.
     */
    bool eggExists(int id) const { return eggs.find(id) != eggs.end(); }

    void purgeDyingPlayers() const
    {
        for (auto it = dyingPlayers.begin(); it != dyingPlayers.end();)
            it = it->second.visual.behaviors.empty() ? dyingPlayers.erase(it) : ++it;
    }
};
