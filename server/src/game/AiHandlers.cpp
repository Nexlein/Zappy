#include <string>

#include "CommandDispatcher.hpp"
#include "core/data/Orientation.hpp"
#include "core/data/Resources.hpp"
#include "protocol/Serializer.hpp"
#include "utils/broadcastDirection.hpp"

void CommandDispatcher::_handleForward(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(std::chrono::milliseconds(7000 / _freq), [this, connectionId, playerId] {
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

        auto& moved = _world.getPlayer(playerId);
        _clients.send(connectionId, "ok\n");
        _notifier.broadcast(Serializer::ppo(moved.id, moved.x, moved.y, moved.orientation));
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleRight(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(std::chrono::milliseconds(7000 / _freq), [this, connectionId, playerId] {
        auto& p = _world.getPlayer(playerId);

        p.orientation = turnRight(p.orientation);
        _clients.send(connectionId, "ok\n");
        _notifier.broadcast(Serializer::ppo(p.id, p.x, p.y, p.orientation));
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleLeft(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(std::chrono::milliseconds(7000 / _freq), [this, connectionId, playerId] {
        auto& p = _world.getPlayer(playerId);
        p.orientation = turnLeft(p.orientation);
        _clients.send(connectionId, "ok\n");
        _notifier.broadcast(Serializer::ppo(p.id, p.x, p.y, p.orientation));
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleLook(int connectionId) {}

void CommandDispatcher::_handleInventory(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(std::chrono::milliseconds(1000 / _freq), [this, connectionId, playerId] {
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

    _scheduler.schedule(std::chrono::milliseconds(7000 / _freq), [this, connectionId, playerId,
                                                                  msg] {
        auto& src = _world.getPlayer(playerId);
        for (auto& [pid, dst] : _world.getPlayers()) {
            if (pid == playerId) continue;
            int dir = broadcastDirection(src.x, src.y, dst.x, dst.y, _world.width(),
                                         _world.height(), dst.orientation);
            _clients.send(dst.connectionId, "message " + std::to_string(dir) + "," + msg + "\n");
        }
        _notifier.broadcast(Serializer::pbc(playerId, msg));
        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleFork(int connectionId) {
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(std::chrono::milliseconds(42000 / _freq), [this, connectionId, playerId] {
        auto& p = _world.getPlayer(playerId);
        int eggId = _world.addEgg(playerId);

        _notifier.broadcast(Serializer::pfk(playerId));
        _notifier.broadcast(Serializer::enw(eggId, playerId, p.x, p.y));
        _clients.send(connectionId, "ok\n");
        _executeNext(connectionId);
    });
}

void CommandDispatcher::_handleEject(int connectionId) {}

void CommandDispatcher::_handleTake(int connectionId, ResourceType resource)
{
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(
        std::chrono::milliseconds(7000 / _freq), [this, connectionId, playerId, resource] {
            auto& p = _world.getPlayer(playerId);

            if (_world.takeResource(playerId, resource)) {
                _clients.send(connectionId, "ok\n");
                _notifier.broadcast(Serializer::pgt(playerId, static_cast<int>(resource)));
                _notifier.broadcast(Serializer::bct(p.x, p.y, _world.at(p.x, p.y).resources));
            } else {
                _clients.send(connectionId, "ko\n");
            }
            _executeNext(connectionId);
        });
}

void CommandDispatcher::_handleSet(int connectionId, ResourceType resource)
{
    int playerId = _clients.getConnection(connectionId).playerId();

    _scheduler.schedule(
        std::chrono::milliseconds(7000 / _freq), [this, connectionId, playerId, resource] {
            auto& p = _world.getPlayer(playerId);

            if (_world.setResource(playerId, resource)) {
                _clients.send(connectionId, "ok\n");
                _notifier.broadcast(Serializer::pdr(playerId, static_cast<int>(resource)));
                _notifier.broadcast(Serializer::bct(p.x, p.y, _world.at(p.x, p.y).resources));
            } else {
                _clients.send(connectionId, "ko\n");
            }
            _executeNext(connectionId);
        });
}

void CommandDispatcher::_handleIncantation(int connectionId) {}
