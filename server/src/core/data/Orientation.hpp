#pragma once

#include <ostream>

/**
 * @brief Represents the orientation of an object in the game.
 * Orientation mapped to integers as required by the protocol.
 * See G-YEP-400_zappy_GUI_protocol.pdf for details.
 */
enum class Orientation { N = 1, E = 2, S = 3, W = 4 };

inline Orientation turnRight(Orientation o)
{
    switch (o) {
        case Orientation::N:
            return Orientation::E;
        case Orientation::E:
            return Orientation::S;
        case Orientation::S:
            return Orientation::W;
        case Orientation::W:
            return Orientation::N;
    }
    return o;
}

inline Orientation turnLeft(Orientation o)
{
    switch (o) {
        case Orientation::N:
            return Orientation::W;
        case Orientation::W:
            return Orientation::S;
        case Orientation::S:
            return Orientation::E;
        case Orientation::E:
            return Orientation::N;
    }
    return o;
}

inline std::ostream& operator<<(std::ostream& os, Orientation orientation)
{
    switch (orientation) {
        case Orientation::N:
            return os << "North";
        case Orientation::E:
            return os << "East";
        case Orientation::S:
            return os << "South";
        case Orientation::W:
            return os << "West";
    }
    return os << "Unknown";
}
