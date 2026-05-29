#pragma once

#include <stdexcept>

class Resources {
public:
    int food;
    int linemate;
    int deraumere;
    int sibur;
    int mendiane;
    int phiras;
    int thystame;

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
