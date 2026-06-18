#include "Serializer.hpp"

static std::string id(int n) { return "#" + std::to_string(n); }
static std::string n(int v) { return std::to_string(v); }
static std::string res(const Resources& r)
{
    return n(r.food) + " " + n(r.linemate) + " " + n(r.deraumere) + " " + n(r.sibur) + " " +
           n(r.mendiane) + " " + n(r.phiras) + " " + n(r.thystame);
}

std::string Serializer::msz(int x, int y) { return "msz " + n(x) + " " + n(y) + "\n"; }

std::string Serializer::bct(int x, int y, const Resources& r)
{
    return "bct " + n(x) + " " + n(y) + " " + res(r) + "\n";
}

std::string Serializer::tna(const std::string& teamName) { return "tna " + teamName + "\n"; }

std::string Serializer::pnw(int pid, int x, int y, Orientation o, int level,
                            const std::string& team)
{
    return "pnw " + id(pid) + " " + n(x) + " " + n(y) + " " + n(static_cast<int>(o)) + " " +
           n(level) + " " + team + "\n";
}

std::string Serializer::ppo(int pid, int x, int y, Orientation o)
{
    return "ppo " + id(pid) + " " + n(x) + " " + n(y) + " " + n(static_cast<int>(o)) + "\n";
}

std::string Serializer::plv(int pid, int level) { return "plv " + id(pid) + " " + n(level) + "\n"; }

std::string Serializer::pin(int pid, int x, int y, const Resources& r)
{
    return "pin " + id(pid) + " " + n(x) + " " + n(y) + " " + res(r) + "\n";
}

std::string Serializer::pex(int pid) { return "pex " + id(pid) + "\n"; }

std::string Serializer::pbc(int pid, const std::string& msg)
{
    return "pbc " + id(pid) + " " + msg + "\n";
}

std::string Serializer::pic(int x, int y, int level, const std::vector<int>& ids)
{
    std::string s = "pic " + n(x) + " " + n(y) + " " + n(level);
    for (int pid : ids) s += " " + id(pid);
    return s + "\n";
}

std::string Serializer::pie(int x, int y, bool success)
{
    return "pie " + n(x) + " " + n(y) + " " + (success ? "1" : "0") + "\n";
}

std::string Serializer::pfk(int pid) { return "pfk " + id(pid) + "\n"; }

std::string Serializer::pdr(int pid, int resourceIndex)
{
    return "pdr " + id(pid) + " " + n(resourceIndex) + "\n";
}

std::string Serializer::pgt(int pid, int resourceIndex)
{
    return "pgt " + id(pid) + " " + n(resourceIndex) + "\n";
}

std::string Serializer::pdi(int pid) { return "pdi " + id(pid) + "\n"; }

std::string Serializer::enw(int eggId, int playerId, int x, int y)
{
    return "enw " + id(eggId) + " " + id(playerId) + " " + n(x) + " " + n(y) + "\n";
}

std::string Serializer::sse(int eggId, const std::string& teamName, int x, int y)
{
    return "sse " + id(eggId) + " " + teamName + " " + n(x) + " " + n(y) + "\n";
}

std::string Serializer::ebo(int eggId) { return "ebo " + id(eggId) + "\n"; }
std::string Serializer::edi(int eggId) { return "edi " + id(eggId) + "\n"; }

std::string Serializer::sgt(int freq) { return "sgt " + n(freq) + "\n"; }
std::string Serializer::sst(int freq) { return "sst " + n(freq) + "\n"; }

std::string Serializer::seg(const std::string& team) { return "seg " + team + "\n"; }
std::string Serializer::smg(const std::string& msg) { return "smg " + msg + "\n"; }

std::string Serializer::suc() { return "suc\n"; }
std::string Serializer::sbp() { return "sbp\n"; }
