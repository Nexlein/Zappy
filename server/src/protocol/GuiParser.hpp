#pragma once

#include <optional>
#include <string_view>
#include <variant>

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

    using Request = std::variant<Msz, Bct, Mct, Tna, Ppo, Plv, Pin, Sgt, Sst>;

}  // namespace Gui

class GuiParser {
    public:
    static std::optional<Gui::Request> parse(std::string_view line);
};
