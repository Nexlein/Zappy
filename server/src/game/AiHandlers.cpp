#include <algorithm>
#include <cctype>
#include <string>

#include "CommandDispatcher.hpp"
#include "core/data/Orientation.hpp"
#include "core/data/Resources.hpp"
#include "utils/broadcastDirection.hpp"

struct LookVectors {
    int fx;
    int fy;
    int rx;
    int ry;
};

static LookVectors _lookForwardRight(Orientation o)
{
    switch (o) {
        case Orientation::N:
            return {0, -1, 1, 0};
        case Orientation::S:
            return {0, 1, -1, 0};
        case Orientation::E:
            return {1, 0, 0, 1};
        case Orientation::W:
            return {-1, 0, 0, -1};
    }
    return {0, -1, 1, 0};
}

static std::string _tileContent(const Tile& tile)
{
    std::string s;
    for (size_t i = 0; i < tile.playerIds.size(); i++) {
        if (!s.empty()) s += " ";
        s += "player";
    }
    for (int res = 0; res < Resources::TYPE_COUNT; res++) {
        auto type = static_cast<ResourceType>(res);
        std::string name = Resources::get_name(type);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        for (int c = 0; c < tile.resources[type]; c++) {
            if (!s.empty()) s += " ";
            s += name;
        }
    }
    return s;
}

void CommandDispatcher::_handleForward(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto& p = _world.getPlayer(playerId);

        int dx = 0, dy = 0;
        switch (p.orientation) {
            case Orientation::N:
                dy = -1;
                break;
            case Orientation::S:
                dy = 1;
                break;
            case Orientation::E:
                dx = 1;
                break;
            case Orientation::W:
                dx = -1;
                break;
        }
        _world.movePlayer(playerId, p.x + dx, p.y + dy);

        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleRight(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto& p = _world.getPlayer(playerId);

        p.orientation = turnRight(p.orientation);
        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleLeft(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto& p = _world.getPlayer(playerId);
        p.orientation = turnLeft(p.orientation);
        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleLook(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto& p = _world.getPlayer(playerId);
        auto look = _lookForwardRight(p.orientation);
        int tileCount = (p.level + 1) * (p.level + 1);

        std::string r = "[";
        for (int i = 0; i < tileCount; i++) {
            int d = 0;
            while ((d + 1) * (d + 1) <= i) d++;
            int xRel = i - 1 * d - d;

            int dx = d * look.fx + xRel * look.rx;
            int dy = d * look.fy + xRel * look.ry;

            if (i > 0) r += ", ";
            r += _tileContent(_world.at(p.x + dx, p.y + dy));
        }
        r += "]\n";

        _clients.send(connectionId, r);
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleInventory(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 1000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto& p = _world.getPlayer(playerId);

        std::string r = "[ food " + std::to_string(p.inventory.food) + ", linemate " +
                        std::to_string(p.inventory.linemate) + ", deraumere " +
                        std::to_string(p.inventory.deraumere) + ", sibur " +
                        std::to_string(p.inventory.sibur) + ", mendiane " +
                        std::to_string(p.inventory.mendiane) + ", phiras " +
                        std::to_string(p.inventory.phiras) + ", thystame " +
                        std::to_string(p.inventory.thystame) + " ]\n";

        _clients.send(connectionId, r);
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleBroadcast(int connectionId, const std::string& msg)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId,
                                                                     msg] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto& src = _world.getPlayer(playerId);
        for (auto& [pid, dst] : _world.getPlayers()) {
            if (pid == playerId) continue;
            int dir = broadcastDirection(src.x, src.y, dst.x, dst.y, _world.width(),
                                         _world.height(), dst.orientation);
            _clients.send(dst.connectionId, "message " + std::to_string(dir) + "," + msg + "\n");
        }
        _world.playerBroadcast(playerId, msg);
        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleFork(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 42000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        _world.addEgg(playerId);
        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleEject(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq), [this, connectionId, playerId] {
        if (!_world.getPlayers().count(playerId)) {
            _executeNext(connectionId);
            return;
        }
        auto result = _world.ejectPlayers(playerId);

        if (result.ejectedPlayerIds.empty()) {
            _clients.send(connectionId, "ko\n");
            _executeNext(connectionId);
            return;
        }

        for (int eid : result.ejectedPlayerIds) {
            auto& ejected = _world.getPlayer(eid);
            int dir =
                broadcastDirection(ejected.x - result.dx, ejected.y - result.dy, ejected.x,
                                   ejected.y, _world.width(), _world.height(), ejected.orientation);
            _clients.send(ejected.connectionId, "eject: " + std::to_string(dir) + "\n");
        }

        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleTake(int connectionId, ResourceType resource)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq),
                        [this, connectionId, playerId, resource] {
                            if (!_world.getPlayers().count(playerId)) {
                                _executeNext(connectionId);
                                return;
                            }
                            if (_world.takeResource(playerId, resource))
                                _clients.send(connectionId, "ok\n");
                            else
                                _clients.send(connectionId, "ko\n");
                            _executeNext(connectionId);
                        });
}

void CommandDispatcher::_handleSet(int connectionId, ResourceType resource)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 7000;

    _scheduler.schedule(std::chrono::milliseconds(delayMs / _freq),
                        [this, connectionId, playerId, resource] {
                            if (!_world.getPlayers().count(playerId)) {
                                _executeNext(connectionId);
                                return;
                            }

                            if (_world.setResource(playerId, resource))
                                _clients.send(connectionId, "ok\n");
                            else
                                _clients.send(connectionId, "ko\n");
                            _executeNext(connectionId);
                        });
}

void CommandDispatcher::_handleIncantation(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    int delayMs = 300000;

    auto participants = _world.startIncantation(playerId);
    if (!participants) {
        _clients.send(connectionId, "ko\n");
        _executeNext(connectionId);
        return;
    }

    auto& p = _world.getPlayer(playerId);
    int x = p.x;
    int y = p.y;

    for (int pid : *participants)
        _clients.send(_world.getPlayer(pid).connectionId, "Elevation underway\n");

    _scheduler.schedule(
        std::chrono::milliseconds(delayMs / _freq),
        [this, connectionId, participants = *participants, x, y] {
            bool success = _world.finalizeIncantation(x, y, participants);

            auto& players = _world.getPlayers();
            for (int pid : participants) {
                auto it = players.find(pid);
                if (it == players.end()) continue;

                if (success) {
                    _clients.send(it->second.connectionId,
                                  "Current level: " + std::to_string(it->second.level) + "\n");
                } else {
                    _clients.send(it->second.connectionId, "ko\n");
                }
            }

            _executeNext(connectionId);
        });
}
