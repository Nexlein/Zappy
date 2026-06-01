#include "ProtocolParser.hpp"

#include <unordered_map>

std::optional<Event> ProtocolParser::parse(std::string_view input)
{
    std::vector<std::string_view> tokens = _split(input, ' ');

    if (tokens.empty()) return std::nullopt;

    // Map command -> parser function
    using ParserFunc = std::optional<Event> (*)(const std::vector<std::string_view>&);
    static const std::unordered_map<std::string_view, ParserFunc> parsers = {
        {"msz", _parseMSZ}, {"bct", _parseBCT}, {"tna", _parseTNA}, {"pnw", _parsePNW},
        {"ppo", _parsePPO}, {"plv", _parsePLV}, {"pin", _parsePIN}, {"pex", _parsePEX},
        {"pbc", _parsePBC}, {"pic", _parsePIC}, {"pie", _parsePIE}, {"pfk", _parsePFK},
        {"pdr", _parsePDR}, {"pgt", _parsePGT}, {"pdi", _parsePDI}, {"enw", _parseENW},
        {"ebo", _parseEBO}, {"edi", _parseEDI}, {"sgt", _parseSGT}, {"sst", _parseSST},
        {"seg", _parseSEG}, {"smg", _parseSMG}, {"suc", _parseSUC}, {"sbp", _parseSBP}};

    auto it = parsers.find(tokens[0]);
    if (it != parsers.end()) {
        return it->second(tokens);
    }

    return std::nullopt;
}

std::vector<std::string_view> ProtocolParser::_split(std::string_view str, char delimiter,
                                                     char stopAt)
{
    std::vector<std::string_view> tokens;

    size_t stopPos = str.find(stopAt);
    if (stopPos == std::string_view::npos) {
        stopPos = str.size();
    }

    size_t start = 0;
    while (start < stopPos) {
        size_t end = str.find(delimiter, start);

        if (end == std::string_view::npos || end >= stopPos) {
            end = stopPos;
        }

        if (end > start) {
            tokens.emplace_back(str.substr(start, end - start));
        }

        start = end + 1;  // Skip delimiter
    }

    return tokens;
}

std::string ProtocolParser::_joinTokens(const std::vector<std::string_view>& tokens, size_t start)
{
    std::string result;
    for (size_t i = start; i < tokens.size(); ++i) {
        if (i > start) result += " ";
        result += tokens[i];
    }
    return result;
}

std::optional<Event> ProtocolParser::_parseMSZ(const std::vector<std::string_view>& tokens)
{
    // msz X Y

    if (tokens.size() != 3) return std::nullopt;

    try {
        int width = std::stoi(std::string(tokens[1]));
        int height = std::stoi(std::string(tokens[2]));
        return MapSize{width, height};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseBCT(const std::vector<std::string_view>& tokens)
{
    // bct X Y q0 q1 q2 q3 q4 q5 q6

    if (tokens.size() != 10) return std::nullopt;

    try {
        int x = std::stoi(std::string(tokens[1]));
        int y = std::stoi(std::string(tokens[2]));

        Resources resources;
        for (size_t i = 0; i < 7; ++i) {
            resources[i] = std::stoi(std::string(tokens[3 + i]));
        }

        return TileContent{x, y, resources};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseTNA(const std::vector<std::string_view>& tokens)
{
    // tna N (N can contain spaces)

    if (tokens.size() < 2) return std::nullopt;

    return TeamName{_joinTokens(tokens, 1)};
}

std::optional<Event> ProtocolParser::_parsePNW(const std::vector<std::string_view>& tokens)
{
    // pnw #n X Y O L N

    if (tokens.size() != 7) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        int x = std::stoi(std::string(tokens[2]));
        int y = std::stoi(std::string(tokens[3]));

        // Orientation is 1-based in the protocol, and our enum is also 1-based, so we can directly
        // cast
        Orientation orientation = static_cast<Orientation>(std::stoi(std::string(tokens[4])));
        int level = std::stoi(std::string(tokens[5]));
        std::string team = std::string(tokens[6]);

        return PlayerNew{id, x, y, orientation, level, team};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePPO(const std::vector<std::string_view>& tokens)
{
    // ppo #n X Y O

    if (tokens.size() != 5) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        int x = std::stoi(std::string(tokens[2]));
        int y = std::stoi(std::string(tokens[3]));

        // Orientation is 1-based in the protocol, and our enum is also 1-based, so we can directly
        // cast
        Orientation orientation = static_cast<Orientation>(std::stoi(std::string(tokens[4])));

        return PlayerPosition{id, x, y, orientation};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePLV(const std::vector<std::string_view>& tokens)
{
    // plv #n L

    if (tokens.size() != 3) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        int level = std::stoi(std::string(tokens[2]));

        return PlayerLevel{id, level};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePIN(const std::vector<std::string_view>& tokens)
{
    // pin #n X Y q0 q1 q2 q3 q4 q5 q6

    if (tokens.size() != 11) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        int x = std::stoi(std::string(tokens[2]));
        int y = std::stoi(std::string(tokens[3]));

        Resources inventory;
        for (size_t i = 0; i < 7; ++i) {
            inventory[i] = std::stoi(std::string(tokens[4 + i]));
        }

        return PlayerInventory{id, x, y, inventory};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePEX(const std::vector<std::string_view>& tokens)
{
    // pex #n

    if (tokens.size() != 2) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        return PlayerExpulsion{id};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePBC(const std::vector<std::string_view>& tokens)
{
    // pbc #n M (M can contain spaces)

    if (tokens.size() < 3) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        return PlayerBroadcast{id, _joinTokens(tokens, 2)};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePIC(const std::vector<std::string_view>& tokens)
{
    // pic X Y L #n1 #n2 ...

    if (tokens.size() < 5) return std::nullopt;

    try {
        int x = std::stoi(std::string(tokens[1]));
        int y = std::stoi(std::string(tokens[2]));
        int level = std::stoi(std::string(tokens[3]));

        std::vector<int> playerIds;
        for (size_t i = 4; i < tokens.size(); ++i) {
            playerIds.push_back(std::stoi(std::string(tokens[i].substr(1))));  // Skip '#'
        }

        return IncantationStart{x, y, level, playerIds};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePIE(const std::vector<std::string_view>& tokens)
{
    // pie X Y R

    if (tokens.size() != 4) return std::nullopt;

    try {
        int x = std::stoi(std::string(tokens[1]));
        int y = std::stoi(std::string(tokens[2]));

        // R is 1 for success, 0 for failure
        bool success = (tokens[3] == "1");

        return IncantationEnd{x, y, success};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePFK(const std::vector<std::string_view>& tokens)
{
    // pfk #n

    if (tokens.size() != 2) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        return PlayerFork{id};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePDR(const std::vector<std::string_view>& tokens)
{
    // pdr #n i

    if (tokens.size() != 3) return std::nullopt;

    try {
        int playerId = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        int resourceId = std::stoi(std::string(tokens[2]));
        return PlayerResourceDrop{playerId, resourceId};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePGT(const std::vector<std::string_view>& tokens)
{
    // pgt #n i

    if (tokens.size() != 3) return std::nullopt;

    try {
        int playerId = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        int resourceId = std::stoi(std::string(tokens[2]));
        return PlayerResourceTake{playerId, resourceId};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parsePDI(const std::vector<std::string_view>& tokens)
{
    // pdi #n

    if (tokens.size() != 2) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        return PlayerDeath{id};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseENW(const std::vector<std::string_view>& tokens)
{
    // enw #e #n X Y

    if (tokens.size() != 5) return std::nullopt;

    try {
        int eggId = std::stoi(std::string(tokens[1].substr(1)));     // Skip '#'
        int playerId = std::stoi(std::string(tokens[2].substr(1)));  // Skip '#'
        int x = std::stoi(std::string(tokens[3]));
        int y = std::stoi(std::string(tokens[4]));

        return EggNew{eggId, playerId, x, y};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseEBO(const std::vector<std::string_view>& tokens)
{
    // ebo #e

    if (tokens.size() != 2) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        return EggHatch{id};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseEDI(const std::vector<std::string_view>& tokens)
{
    // edi #e

    if (tokens.size() != 2) return std::nullopt;

    try {
        int id = std::stoi(std::string(tokens[1].substr(1)));  // Skip '#'
        return EggDeath{id};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseSGT(const std::vector<std::string_view>& tokens)
{
    // sgt T

    if (tokens.size() != 2) return std::nullopt;

    try {
        int timeUnit = std::stoi(std::string(tokens[1]));
        return TimeUnit{timeUnit};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseSST(const std::vector<std::string_view>& tokens)
{
    // sst T

    if (tokens.size() != 2) return std::nullopt;

    try {
        int timeUnit = std::stoi(std::string(tokens[1]));
        return TimeUnitChange{timeUnit};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Event> ProtocolParser::_parseSEG(const std::vector<std::string_view>& tokens)
{
    // seg N (N can contain spaces)

    if (tokens.size() < 2) return std::nullopt;

    return GameEnd{_joinTokens(tokens, 1)};
}

std::optional<Event> ProtocolParser::_parseSMG(const std::vector<std::string_view>& tokens)
{
    // smg M (M can contain spaces)

    if (tokens.size() < 2) return std::nullopt;

    return ServerMessage{_joinTokens(tokens, 1)};
}

std::optional<Event> ProtocolParser::_parseSUC(const std::vector<std::string_view>& tokens)
{
    // suc

    if (tokens.size() != 1) return std::nullopt;

    return UnknownCommand{};
}

std::optional<Event> ProtocolParser::_parseSBP(const std::vector<std::string_view>& tokens)
{
    // sbp

    if (tokens.size() != 1) return std::nullopt;

    return BadParameters{};
}
