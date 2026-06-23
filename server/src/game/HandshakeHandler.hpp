#pragma once

#include <functional>
#include <string>

#include "core/Args.hpp"
#include "core/GameClock.hpp"
#include "core/World.hpp"
#include "game/GuiNotifier.hpp"
#include "network/ClientManager.hpp"

using PromotedCallback = std::function<void(int connectionId, int playerId)>;

/**
 * @brief Handles the first exchange with a new socket.
 *
 * On connect the server sends "WELCOME". The client replies with a team name:
 * "GRAPHIC" becomes a GUI, any other name joins that team as an AI (if a slot
 * is free), anything else is rejected. Once accepted, the connection is
 * promoted to its real type and onPromoted is fired.
 */
class HandshakeHandler {
    public:
    HandshakeHandler(ClientManager& clients, World& world, GuiNotifier& notifier,
                     const ServerConfig& config, const GameClock& clock,
                     PromotedCallback onPromoted);

    /// New socket: send the "WELCOME" greeting.
    void onNewConnection(int connectionId);
    /// The client's reply (its team name).
    void onLine(int connectionId, const std::string& line);

    private:
    void _promoteToGui(int connectionId);
    void _promoteToAi(int connectionId, const std::string& teamName);
    void _reject(int connectionId);

    ClientManager& _clients;
    World& _world;
    GuiNotifier& _notifier;
    const ServerConfig& _config;
    const GameClock& _clock;
    PromotedCallback _onPromoted;
};
