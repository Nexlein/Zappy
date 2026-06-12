#include "CommandDispatcher.hpp"

#include "protocol/GuiParser.hpp"
#include "utils/overloaded.hpp"

CommandDispatcher::CommandDispatcher(ClientManager& clients, World& world, GuiNotifier& notifier,
                                     const ServerConfig& config, Scheduler& scheduler)
    : _clients(clients),
      _world(world),
      _notifier(notifier),
      _scheduler(scheduler),
      _config(config),
      _handshakeHandler(clients, world, notifier, config),
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
    if (conn.type() == ClientType::GUI) _notifier.removeGui(connectionId);
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
    (void)connectionId;
    (void)line;
}

void CommandDispatcher::_executeNext(int connectionId) { (void)connectionId; }
