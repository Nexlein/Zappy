#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "../core/data/Resources.hpp"

/**
 * @brief Typed structs for the 12 AI client commands.
 *
 * Zero-arg commands are empty structs. Take and Set carry a ResourceType
 * parsed from the wire name ("linemate", "food"...). Broadcast carries the
 * full message including spaces.
 *
 * @see G-YEP-400_zappy.pdf for the AI command reference.
 * @see Resources::fromName() for wire name to ResourceType conversion.
 */
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

/**
 * @brief Parses a raw AI client line into a typed Ai::Command.
 *
 * Strict and case-sensitive. Returns nullopt on unknown or malformed input.
 * The caller (CommandDispatcher) handles sending "ko\n" back to the client.
 *
 * @see G-YEP-400_zappy.pdf for syntax of each command.
 */
class AiParser {
    public:
    /**
     * @brief Parse one line from an AI client (no trailing newline).
     * @return Parsed command, or nullopt if unknown or malformed.
     */
    static std::optional<Ai::Command> parse(std::string_view line);
};
