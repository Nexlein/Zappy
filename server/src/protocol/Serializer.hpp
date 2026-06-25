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
 *
 * @note Wire conventions used below:
 *       - `#N`   player or egg id, always prefixed with '#' (e.g. "#3").
 *       - `O`    orientation as an int 1..4 (N=1, E=2, S=3, W=4).
 *       - `q...` a resource block: 7 space-separated quantities in fixed order
 *                `food linemate deraumere sibur mendiane phiras thystame`.
 *       - every string ends with a trailing '\n'.
 */
namespace Serializer {
    /// Map size.                       Wire: `msz X Y\n`
    std::string msz(int x, int y);
    /// Tile content (resources on it).  Wire: `bct X Y q q q q q q q\n`
    std::string bct(int x, int y, const Resources& r);
    /// Team name (one per team).        Wire: `tna NAME\n`
    std::string tna(const std::string& teamName);

    /// New player connected.            Wire: `pnw #N X Y O L NAME\n` (L = level)
    std::string pnw(int id, int x, int y, Orientation o, int level, const std::string& team);
    /// Player position.                 Wire: `ppo #N X Y O\n`
    std::string ppo(int id, int x, int y, Orientation o);
    /// Player level.                    Wire: `plv #N L\n`
    std::string plv(int id, int level);
    /// Player inventory.                Wire: `pin #N X Y q q q q q q q\n`
    std::string pin(int id, int x, int y, const Resources& r);
    /// Player expelled (got ejected).   Wire: `pex #N\n`
    std::string pex(int id);
    /// Player broadcast.                Wire: `pbc #N MESSAGE\n`
    std::string pbc(int id, const std::string& msg);
    /// Incantation start. @p ids = all participating player IDs.
    /// Wire: `pic X Y L #N1 #N2 ...\n`
    std::string pic(int x, int y, int level, const std::vector<int>& ids);
    /// Incantation end. @p success maps to `1` (success) or `0` (failure).
    /// Wire: `pie X Y R\n`
    std::string pie(int x, int y, bool success);
    /// Player lays an egg (fork).       Wire: `pfk #N\n`
    std::string pfk(int id);
    /// Player drops resource @p resourceIndex (0..6). Wire: `pdr #N i\n`
    std::string pdr(int id, int resourceIndex);
    /// Player takes resource @p resourceIndex (0..6). Wire: `pgt #N i\n`
    std::string pgt(int id, int resourceIndex);
    /// Player death.                    Wire: `pdi #N\n`
    std::string pdi(int id);

    /// Egg laid by a player.            Wire: `enw #E #N X Y\n` (E = egg, N = parent)
    std::string enw(int eggId, int playerId, int x, int y);
    /// CUSTOM (non-standard): announces an initial team egg spawned at game start.
    /// Wire: `sse #E NAME X Y\n`
    std::string sse(int eggId, const std::string& teamName, int x, int y);
    /// Egg hatched, player connects from it. Wire: `ebo #E\n`
    std::string ebo(int eggId);
    /// Egg died.                        Wire: `edi #E\n`
    std::string edi(int eggId);

    /// Reply to "sgt": current time unit frequency. Wire: `sgt F\n`
    std::string sgt(int freq);
    /// Reply to "sst": frequency was changed to F.   Wire: `sst F\n`
    std::string sst(int freq);
    /// CUSTOM (non-standard): elapsed game time. Wire: `stu SECONDS TICKS\n`
    /// SECONDS is wall uptime (freq-independent), TICKS is freq-integrated game time.
    std::string stu(int seconds, long long ticks);
    /// CUSTOM (non-standard): authoritative time @p team took to win, sent once
    /// right after `seg`. Wire: `gwt NAME SECONDS TICKS\n`.
    std::string gwt(const std::string& team, int seconds, long long ticks);

    /// End of game, @p team won.        Wire: `seg NAME\n`
    std::string seg(const std::string& team);
    /// Server message to all GUIs.      Wire: `smg MESSAGE\n`
    std::string smg(const std::string& msg);
    /// Unknown command from GUI (sent on unparseable line). Wire: `suc\n`
    std::string suc();
    /// Bad command parameter from GUI.  Wire: `sbp\n`
    std::string sbp();
}  // namespace Serializer
