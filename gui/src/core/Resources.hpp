#pragma once

#include <string>
#include <ostream>
#include <stdexcept>

/**
 * @brief Represents the resources available in the game, including food and various minerals.
 */
class Resources {
    public:
    int food = 0;
    int linemate = 0;
    int deraumere = 0;
    int sibur = 0;
    int mendiane = 0;
    int phiras = 0;
    int thystame = 0;

    std::string get_name(int index) const
    {
        switch (index) {
            case 0:
                return "Food";
            case 1:
                return "Linemate";
            case 2:
                return "Deraumere";
            case 3:
                return "Sibur";
            case 4:
                return "Mendiane";
            case 5:
                return "Phiras";
            case 6:
                return "Thystame";
            default:
                throw std::out_of_range("Invalid resource index");
        }
    }

    int& operator[](size_t index)
    {
        switch (index) {
            case 0:
                return food;
            case 1:
                return linemate;
            case 2:
                return deraumere;
            case 3:
                return sibur;
            case 4:
                return mendiane;
            case 5:
                return phiras;
            case 6:
                return thystame;
            default:
                throw std::out_of_range("Invalid resource index");
        }
    }

    const int& operator[](size_t index) const
    {
        switch (index) {
            case 0:
                return food;
            case 1:
                return linemate;
            case 2:
                return deraumere;
            case 3:
                return sibur;
            case 4:
                return mendiane;
            case 5:
                return phiras;
            case 6:
                return thystame;
            default:
                throw std::out_of_range("Invalid resource index");
        }
    }
};

inline std::ostream& operator<<(std::ostream& os, const Resources& res)
{
    os << "Food: " << res.food << ", Linemate: " << res.linemate << ", Deraumere: " << res.deraumere
       << ", Sibur: " << res.sibur << ", Mendiane: " << res.mendiane << ", Phiras: " << res.phiras
       << ", Thystame: " << res.thystame;
    return os;
}
