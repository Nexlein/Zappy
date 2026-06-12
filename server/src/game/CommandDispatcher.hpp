#pragma once

#include "core/Scheduler.hpp"
#include "core/World.hpp"
#include "game/GuiNotifier.hpp"
#include "game/HandshakeHandler.hpp"
#include "network/ClientManager.hpp"

class CommandDispatcher {
    public:
    CommandDispatcher(ClientManager& clients, World& world, GuiNotifier& notifier,
                      const ServerConfig& config, Scheduler& scheduler);

    void onNewConnection(int connectionId);
    void dispatch(int connectionId, const std::string& line);
    void onDisconnect(int connectionId);

    private:
    void _dispatchAi(int playerId, const std::string& line);
    void _dispatchGui(int connectionId, const std::string& line);

    ClientManager& _clients;
    World& _world;
    HandshakeHandler _handshakeHandler;
    GuiNotifier& _notifier;
    Scheduler& _scheduler;
};
