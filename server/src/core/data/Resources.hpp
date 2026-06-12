#pragma once

#include <ostream>
#include <stdexcept>
#include <string>

/**
 * @brief Enumerate the different types of resources in the game.
 */
enum class ResourceType { FOOD = 0, LINEMATE, DERAUMERE, SIBUR, MENDIANE, PHIRAS, THYSTAME, COUNT };

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

    bool isEmpty() const
    {
        return food == 0 && linemate == 0 && deraumere == 0 && sibur == 0 && mendiane == 0 &&
               phiras == 0 && thystame == 0;
    }

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

    std::string get_name(ResourceType type) const { return get_name(static_cast<int>(type)); }

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

    int& operator[](ResourceType type) { return (*this)[static_cast<size_t>(type)]; }

    const int& operator[](ResourceType type) const { return (*this)[static_cast<size_t>(type)]; }

    static float density(ResourceType type)
    {
        switch (type) {
            case ResourceType::FOOD:
                return 0.5f;
            case ResourceType::LINEMATE:
                return 0.3f;
            case ResourceType::DERAUMERE:
                return 0.15f;
            case ResourceType::SIBUR:
                return 0.1f;
            case ResourceType::MENDIANE:
                return 0.1f;
            case ResourceType::PHIRAS:
                return 0.08f;
            case ResourceType::THYSTAME:
                return 0.05f;
            default:
                throw std::out_of_range("Invalid resource type");
        }
    }
    static constexpr int TYPE_COUNT = static_cast<int>(ResourceType::COUNT);
};

inline std::ostream& operator<<(std::ostream& os, const Resources& res)
{
    os << "Food: " << res.food << ", Linemate: " << res.linemate << ", Deraumere: " << res.deraumere
       << ", Sibur: " << res.sibur << ", Mendiane: " << res.mendiane << ", Phiras: " << res.phiras
       << ", Thystame: " << res.thystame;
    return os;
}
