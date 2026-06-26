#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <variant>

/**
 * @brief Typed structs for the GUI client request commands.
 *
 * Player-targeting requests (ppo, plv, pin) carry the player id after
 * stripping the '#' prefix from the wire format.
 *
 * @see G-YEP-400_zappy_GUI_protocol.pdf, "Client" column.
 * @see Serializer for the server responses triggered by these requests.
 */
namespace Gui {
    struct Msz {};
    struct Bct {
        int x, y;
    };
    struct Mct {};
    struct Tna {};
    struct Ppo {
        int id;
    };
    struct Plv {
        int id;
    };
    struct Pin {
        int id;
    };
    struct Sgt {};
    struct Sst {
        int freq;
    };
    struct Stu {};

    using Request = std::variant<Msz, Bct, Mct, Tna, Ppo, Plv, Pin, Sgt, Sst, Stu>;
}  // namespace Gui

/**
 * @brief Parses a raw GUI client line into a typed Gui::Request.
 *
 * Strict and case-sensitive. Returns nullopt on malformed input (missing
 * args, bad integers, missing '#' prefix). Same policy as AiParser.
 *
 * @see G-YEP-400_zappy_GUI_protocol.pdf for syntax.
 */
class GuiParser {
    public:
    /**
     * @brief Parse one line from a GUI client (no trailing newline).
     * @return Parsed request, or nullopt if unknown or malformed.
     */
    static std::optional<Gui::Request> parse(std::string_view line);
};
