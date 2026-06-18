#pragma once

#include <string>
#include <vector>

#include "core/data/Orientation.hpp"
#include "core/data/Resources.hpp"

/**
 * @brief Generates all 24 server-to-GUI wire strings.
 *
 * Inverse of the GUI's ProtocolParser. All functions are pure -- they just
 * return a formatted string with a trailing newline. Broadcasting to sockets
 * is GuiNotifier's job. Function names match the protocol commands 1:1.
 *
 * @see G-YEP-400_zappy_GUI_protocol.pdf, "Server" column for all wire formats.
 * @see GuiNotifier for how these strings reach GUI clients.
 */
namespace Serializer {
    std::string msz(int x, int y);
    std::string bct(int x, int y, const Resources& r);
    std::string tna(const std::string& teamName);

    std::string pnw(int id, int x, int y, Orientation o, int level, const std::string& team);
    std::string ppo(int id, int x, int y, Orientation o);
    std::string plv(int id, int level);
    std::string pin(int id, int x, int y, const Resources& r);
    std::string pex(int id);
    std::string pbc(int id, const std::string& msg);
    /// @p ids is the list of all participating player IDs.
    std::string pic(int x, int y, int level, const std::vector<int>& ids);
    /// @p success maps to R=1 (success) or R=0 (failure) in the wire format.
    std::string pie(int x, int y, bool success);
    std::string pfk(int id);
    std::string pdr(int id, int resourceIndex);
    std::string pgt(int id, int resourceIndex);
    std::string pdi(int id);

    std::string enw(int eggId, int playerId, int x, int y);
    std::string sse(int eggId, const std::string& teamName, int x, int y);
    std::string ebo(int eggId);
    std::string edi(int eggId);

    std::string sgt(int freq);
    std::string sst(int freq);

    std::string seg(const std::string& team);
    std::string smg(const std::string& msg);
    std::string suc();
    std::string sbp();
}  // namespace Serializer
