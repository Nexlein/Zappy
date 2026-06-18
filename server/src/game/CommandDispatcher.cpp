#include "CommandDispatcher.hpp"

#include <chrono>
#include <variant>

#include "protocol/AiParser.hpp"
#include "protocol/GuiParser.hpp"
#include "utils/overloaded.hpp"

CommandDispatcher::CommandDispatcher(ClientManager& clients, World& world, GuiNotifier& notifier,
                                     const ServerConfig& config, Scheduler& scheduler)
    : _clients(clients),
      _world(world),
      _notifier(notifier),
      _scheduler(scheduler),
      _config(config),
      _handshakeHandler(clients, world, notifier, config,
                        [this](int connectionId, int playerId) {
                            this->_startStarvationTimer(connectionId, playerId);
                        }),
      _freq(config.freq)
{
}

void CommandDispatcher::onNewConnection(int connectionId)
{
    _handshakeHandler.onNewConnection(connectionId);
}

void CommandDispatcher::dispatch(int connectionId, const std::string& line)
{
    auto& conn = _clients.getConnection(connectionId);
    if (conn.type() == ClientType::PENDING)
        _handshakeHandler.onLine(connectionId, line);
    else if (conn.type() == ClientType::AI)
        _dispatchAi(connectionId, line);
    else if (conn.type() == ClientType::GUI)
        _dispatchGui(connectionId, line);
}

void CommandDispatcher::onDisconnect(int connectionId)
{
    auto& conn = _clients.getConnection(connectionId);
    if (conn.type() == ClientType::GUI) {
        _notifier.removeGui(connectionId);
    } else if (conn.type() == ClientType::AI) {
        int playerId = conn.playerId();
        if (_world.getPlayers().count(playerId)) {
            _world.removePlayer(playerId);
        }
    }
    _queues.erase(connectionId);
    _hasActive.erase(connectionId);
}

void CommandDispatcher::_dispatchGui(int connectionId, const std::string& line)
{
    auto req = GuiParser::parse(line);
    if (!req) {
        _clients.send(connectionId, "suc\n");
        return;
    }

    std::visit(overloaded{
                   [&](Gui::Msz) { _handleMsz(connectionId); },
                   [&](Gui::Bct c) { _handleBct(connectionId, c.x, c.y); },
                   [&](Gui::Mct) { _handleMct(connectionId); },
                   [&](Gui::Tna) { _handleTna(connectionId); },
                   [&](Gui::Ppo p) { _handlePpo(connectionId, p.id); },
                   [&](Gui::Plv p) { _handlePlv(connectionId, p.id); },
                   [&](Gui::Pin p) { _handlePin(connectionId, p.id); },
                   [&](Gui::Sgt) { _handleSgt(connectionId); },
                   [&](Gui::Sst s) { _handleSst(s.freq); },
                   [&](auto&) { _clients.send(connectionId, "suc\n"); },
               },
               *req);
}

void CommandDispatcher::_dispatchAi(int connectionId, const std::string& line)
{
    auto cmd = AiParser::parse(line);
    if (!cmd) {
        _clients.send(connectionId, "ko\n");
        return;
    }

    if (_queues[connectionId].size() >= 10) return;
    _queues[connectionId].push_back(*cmd);
    if (!_hasActive[connectionId]) _executeNext(connectionId);
}

void CommandDispatcher::_executeNext(int connectionId)
{
    auto& queue = _queues[connectionId];
    if (queue.empty()) {
        _hasActive[connectionId] = false;
        return;
    }

    _hasActive[connectionId] = true;
    Ai::Command cmd = queue.front();
    queue.pop_front();

    std::visit(overloaded{
                   [&](Ai::Forward) { _handleForward(connectionId); },
                   [&](Ai::Right) { _handleRight(connectionId); },
                   [&](Ai::Left) { _handleLeft(connectionId); },
                   [&](Ai::Look) { _handleLook(connectionId); },
                   [&](Ai::Inventory) { _handleInventory(connectionId); },
                   [&](Ai::Broadcast& b) { _handleBroadcast(connectionId, b.message); },
                   [&](Ai::Fork) { _handleFork(connectionId); },
                   [&](Ai::Eject) { _handleEject(connectionId); },
                   [&](Ai::Take& t) { _handleTake(connectionId, t.resource); },
                   [&](Ai::Set& s) { _handleSet(connectionId, s.resource); },
                   [&](Ai::Incantation) { _handleIncantation(connectionId); },
                   [&](Ai::ConnectNbr) { _handleConnectNbr(connectionId); },
               },
               cmd);
}

void CommandDispatcher::_handleConnectNbr(int connectionId)
{
    int playerId = _clients.getConnection(connectionId).playerId();
    if (_world.getPlayers().count(playerId)) {
        auto& player = _world.getPlayer(playerId);
        int slots = _config.clientsNb - _world.teamPlayerCount(player.teamName);
        _clients.send(connectionId, std::to_string(slots) + "\n");
    }
    _executeNext(connectionId);
}

void CommandDispatcher::_startStarvationTimer(int connectionId, int playerId)
{
    _scheduler.schedule(std::chrono::milliseconds(STARVATION_INTERVAL_MS) / _freq,
                        [this, connectionId, playerId] {
                            if (!_world.getPlayers().count(playerId)) return;
                            auto& p = _world.getPlayer(playerId);
                            p.inventory.food--;
                            if (p.inventory.food <= 0) {
                                _world.removePlayer(playerId);
                                _clients.send(connectionId, "dead\n");
                                _pendingDisconnects.push_back(connectionId);
                            } else {
                                _startStarvationTimer(connectionId, playerId);
                            }
                        });
}

std::vector<int> CommandDispatcher::drainPendingDisconnects()
{
    std::vector<int> result;
    std::swap(result, _pendingDisconnects);
    return result;
}