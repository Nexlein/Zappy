#pragma once

#include <string>
#include <vector>

#include "core/data/Orientation.hpp"
#include "core/data/Resources.hpp"

namespace Serializer {
    // map
    std::string msz(int x, int y);
    std::string bct(int x, int y, const Resources& r);
    std::string tna(const std::string& teamName);

    // players
    std::string pnw(int id, int x, int y, Orientation o, int level, const std::string& team);
    std::string ppo(int id, int x, int y, Orientation o);
    std::string plv(int id, int level);
    std::string pin(int id, int x, int y, const Resources& r);
    std::string pex(int id);
    std::string pbc(int id, const std::string& msg);
    std::string pic(int x, int y, int level, const std::vector<int>& ids);
    std::string pie(int x, int y, bool success);
    std::string pfk(int id);
    std::string pdr(int id, int resourceIndex);
    std::string pgt(int id, int resourceIndex);
    std::string pdi(int id);

    // eggs
    std::string enw(int eggId, int playerId, int x, int y);
    std::string ebo(int eggId);
    std::string edi(int eggId);

    // time
    std::string sgt(int freq);
    std::string sst(int freq);

    // game
    std::string seg(const std::string& team);
    std::string smg(const std::string& msg);
    std::string suc();
    std::string sbp();
};  // namespace Serializer
