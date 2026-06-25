#include "AAudioManager.hpp"

#include <type_traits>
#include <variant>

void AAudioManager::handleEvent(const Event& event)
{
    std::visit(
        [this](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, PlayerDeath>) {
                playSound("death", "gui/assets/sounds/minecraft-villager-death.mp3");
            } else if constexpr (std::is_same_v<T, IncantationStart>) {
                playSound("incantation", "gui/assets/sounds/minecraft-villager.mp3");
            } else if constexpr (std::is_same_v<T, IncantationEnd>) {
                if (arg.success) {
                    playSound("incantation_success", "gui/assets/sounds/minecraft-villager-trade.mp3");
                } else {
                    playSound("incantation_fail", "gui/assets/sounds/minecraft-hit.mp3");
                }
            } else if constexpr (std::is_same_v<T, PlayerLevel>) {
                playSound("levelup", "gui/assets/sounds/minecraft-levelup.mp3");
            } else if constexpr (std::is_same_v<T, PlayerBroadcast>) {
                playSound("broadcast", "gui/assets/sounds/discord.mp3");
            } else if constexpr (std::is_same_v<T, PlayerNew>) {
                playSound("newplayer", "gui/assets/sounds/discord-join.mp3");
            }
            // else if constexpr (std::is_same_v<T, PlayerResourceDrop>) {
            //     playSound("drop", "gui/assets/sounds/minecraft-drop.mp3");
            // } else if constexpr (std::is_same_v<T, PlayerResourceTake>) {
            //     playSound("take", "gui/assets/sounds/minecraft-take.mp3");
            // } else if constexpr (std::is_same_v<T, EggNew>) {
            //     playSound("eggnew", "gui/assets/sounds/minecraft-egg.mp3");
            // } else if constexpr (std::is_same_v<T, EggHatch>) {
            //     playSound("egghatch", "gui/assets/sounds/minecraft-egg-hatch.mp3");
            // } else if constexpr (std::is_same_v<T, EggDeath>) {
            //     playSound("eggdeath", "gui/assets/sounds/minecraft-egg-death.mp3");
            // } else if constexpr (std::is_same_v<T, GameEnd>) {
            //     playSound("gameend", "gui/assets/sounds/minecraft-game-end.mp3");
            // }
        },
        event);
}
