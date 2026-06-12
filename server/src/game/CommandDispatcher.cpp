#include "CommandDispatcher.hpp"

CommandDispatcher::CommandDispatcher(ClientManager& clients, World& world, GuiNotifier& notifier,
                                     const ServerConfig& config, Scheduler& scheduler)
    : _clients(clients),
      _world(world),
      _handshakeHandler(clients, world, notifier, config),
      _notifier(notifier),
      _scheduler(scheduler)
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
}

void CommandDispatcher::_dispatchAi(int playerId, const std::string& line)
{
    (void)playerId;
    (void)line;
}

void CommandDispatcher::_dispatchGui(int connectionId, const std::string& line)
{
    (void)connectionId;
    (void)line;
}