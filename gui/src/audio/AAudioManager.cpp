#include "AAudioManager.hpp"

#include <type_traits>
#include <variant>

void AAudioManager::handleEvent(const Event& event)
{
    std::visit(
        [this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, PlayerDeath>) {
                playSound("death", "gui/assets/sounds/roblox-death.mp3");
            } else if constexpr (std::is_same_v<T, IncantationStart>) {
                playSound("incantation", "gui/assets/sounds/microwave.mp3");
            }
        },
        event);
}
