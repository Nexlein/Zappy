#pragma once

#include <functional>
#include <string>

#include "core/Args.hpp"
#include "core/World.hpp"
#include "game/GuiNotifier.hpp"
#include "network/ClientManager.hpp"

using PromotedCallback = std::function<void(int connectionId, int playerId)>;

class HandshakeHandler {
    public:
    HandshakeHandler(ClientManager& clients, World& world, GuiNotifier& notifier,
                     const ServerConfig& config, PromotedCallback onPromoted);

    void onNewConnection(int connectionId);
    void onLine(int connectionId, const std::string& line);

    private:
    void _promoteToGui(int connectionId);
    void _promoteToAi(int connectionId, const std::string& teamName);
    void _reject(int connectionId);

    ClientManager& _clients;
    World& _world;
    GuiNotifier& _notifier;
    const ServerConfig& _config;
    PromotedCallback _onPromoted;
};
