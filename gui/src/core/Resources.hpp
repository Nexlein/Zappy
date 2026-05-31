#pragma once

#include <ostream>
#include <stdexcept>

class Resources {
public:
    int food = 0;
    int linemate = 0;
    int deraumere = 0;
    int sibur = 0;
    int mendiane = 0;
    int phiras = 0;
    int thystame = 0;

    int& operator[](size_t index) {
        switch (index) {
            case 0: return food;
            case 1: return linemate;
            case 2: return deraumere;
            case 3: return sibur;
            case 4: return mendiane;
            case 5: return phiras;
            case 6: return thystame;
            default: throw std::out_of_range("Invalid resource index");
        }
    }
};

inline std::ostream& operator<<(std::ostream& os, const Resources& res) {
    os << "Food: " << res.food
       << ", Linemate: " << res.linemate
       << ", Deraumere: " << res.deraumere
       << ", Sibur: " << res.sibur
       << ", Mendiane: " << res.mendiane
       << ", Phiras: " << res.phiras
       << ", Thystame: " << res.thystame;
    return os;
}
