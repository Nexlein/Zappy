#pragma once

#include <string>

/**
 * @brief Represents an egg in the game, which has an ID, position (x, y), and it's associated with
 * a team name.
 */
struct Egg {
    int id;
    int parentPlayerId;
    int x;
    int y;
    std::string teamName;
};