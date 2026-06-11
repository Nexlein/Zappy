#pragma once

#include <deque>
#include <string>

#include "Orientation.hpp"
#include "Resources.hpp"

/**
 * @brief Represents a player in the game
 * Each player has an ID, position (x, y), orientation, level, team name, inventory of resources,
 * and a command queue. The player can also be in the process of incanting (see the subject cuh) the
 * max commands a player can queue is 10
 */
struct Player {
    int id;
    int x;
    int y;
    int connectionId;
    Orientation orientation;
    int level = 1;
    std::string teamName;
    Resources inventory;
    bool isIncanting = false;

    std::deque<std::string> commandQueue;
    bool hasActiveAction = false;
};