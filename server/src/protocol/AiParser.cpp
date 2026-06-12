#include "AiParser.hpp"

std::optional<Ai::Command> AiParser::parse(std::string_view line)
{
    if (line == "Incantation") return Ai::Incantation{};
    if (line == "Forward") return Ai::Forward{};
    if (line == "Right") return Ai::Right{};
    if (line == "Left") return Ai::Left{};
    if (line == "Look") return Ai::Look{};
    if (line == "Fork") return Ai::Fork{};
    if (line == "Eject") return Ai::Eject{};
    if (line == "Inventory") return Ai::Inventory{};
    if (line == "Connect_nbr") return Ai::ConnectNbr{};

    if (line.starts_with("Broadcast ")) return Ai::Broadcast{std::string(line.substr(10))};
    if (line.starts_with("Take ")) {
        auto res = Resources::fromName(line.substr(5));
        if (!res) return std::nullopt;
        return Ai::Take{*res};
    }
    if (line.starts_with("Set ")) {
        auto res = Resources::fromName(line.substr(4));
        if (!res) return std::nullopt;
        return Ai::Set{*res};
    }

    return std::nullopt;
}