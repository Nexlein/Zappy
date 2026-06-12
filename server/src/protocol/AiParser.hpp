#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "../core/data/Resources.hpp"

namespace Ai {
    struct Forward {};
    struct Right {};
    struct Left {};
    struct Look {};
    struct Inventory {};
    struct Broadcast {
        std::string message;
    };
    struct ConnectNbr {};
    struct Fork {};
    struct Eject {};
    struct Take {
        ResourceType resource;
    };
    struct Set {
        ResourceType resource;
    };
    struct Incantation {};

    using Command = std::variant<Forward, Right, Left, Look, Inventory, Broadcast, ConnectNbr, Fork,
                                 Eject, Take, Set, Incantation>;
}  // namespace Ai

class AiParser {
    public:
    static std::optional<Ai::Command> parse(std::string_view line);
};