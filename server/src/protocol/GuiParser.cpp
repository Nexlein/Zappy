#include "GuiParser.hpp"

#include <stdexcept>
#include <string>

static int parseInt(std::string_view sv) { return std::stoi(std::string(sv)); }

static int parsePlayerId(std::string_view sv)
{
    if (sv.empty() || sv[0] != '#') throw std::invalid_argument("missing #");
    return std::stoi(std::string(sv.substr(1)));
}

std::optional<Gui::Request> GuiParser::parse(std::string_view line)
{
    if (line == "msz") return Gui::Msz{};
    if (line == "mct") return Gui::Mct{};
    if (line == "tna") return Gui::Tna{};
    if (line == "sgt") return Gui::Sgt{};
    if (line == "stu") return Gui::Stu{};

    if (line.starts_with("bct ")) {
        auto rest = line.substr(4);
        auto sp = rest.find(' ');
        if (sp == std::string_view::npos) return std::nullopt;
        try {
            int x = parseInt(rest.substr(0, sp));
            int y = parseInt(rest.substr(sp + 1));
            return Gui::Bct{x, y};
        } catch (...) {
            return std::nullopt;
        }
    }
    if (line.starts_with("ppo ")) {
        try {
            return Gui::Ppo{parsePlayerId(line.substr(4))};
        } catch (...) {
            return std::nullopt;
        }
    }
    if (line.starts_with("plv ")) {
        try {
            return Gui::Plv{parsePlayerId(line.substr(4))};
        } catch (...) {
            return std::nullopt;
        }
    }
    if (line.starts_with("pin ")) {
        try {
            return Gui::Pin{parsePlayerId(line.substr(4))};
        } catch (...) {
            return std::nullopt;
        }
    }
    if (line.starts_with("sst ")) {
        try {
            return Gui::Sst{parseInt(line.substr(4))};
        } catch (...) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}
