#pragma once

#include <ostream>

/**
 * @brief Represents the orientation of an object in the game.
 * Orientation mapped to integers as required by the protocol.
 * See G-YEP-400_zappy_GUI_protocol.pdf for details.
 */
enum class Orientation {
    N = 1,
    E = 2,
    S = 3,
    W = 4
};

inline std::ostream& operator<<(std::ostream& os, Orientation orientation) {
    switch (orientation) {
        case Orientation::N: return os << "North";
        case Orientation::E: return os << "East";
        case Orientation::S: return os << "South";
        case Orientation::W: return os << "West";
    }
    return os << "Unknown";
}
