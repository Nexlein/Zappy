#pragma once

#include <ostream>

/**
 * @brief Represents the orientation of an object in the game.
 * Orientation mapped to integers as required by the protocol.
 * See G-YEP-400_zappy_GUI_protocol.pdf for details.
 */
enum class Orientation { N = 1, E = 2, S = 3, W = 4 };

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

inline std::string to_string(Orientation orientation)
{
    switch (orientation) {
        case Orientation::N:
            return "North";
        case Orientation::E:
            return "East";
        case Orientation::S:
            return "South";
        case Orientation::W:
            return "West";
    }
    return "Unknown";
}

/**
 * @brief Returns the Y rotation angle (degrees) for a given orientation.
 * @note Specific to the rimuru player model, which faces West at 0°.
 */
inline float toAngle(Orientation orientation)
{
    switch (orientation) {
        case Orientation::W:
            return 0.0f;
        case Orientation::S:
            return 90.0f;
        case Orientation::E:
            return 180.0f;
        case Orientation::N:
            return 270.0f;
    }
    return 0.0f;
}