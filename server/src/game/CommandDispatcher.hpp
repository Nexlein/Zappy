#pragma once

#include <deque>
#include <unordered_map>

#include "core/Scheduler.hpp"
#include "core/World.hpp"
#include "game/GuiNotifier.hpp"
#include "game/HandshakeHandler.hpp"
#include "network/ClientManager.hpp"
#include "protocol/AiParser.hpp"

class CommandDispatcher {
    public:
    CommandDispatcher(ClientManager& clients, World& world, GuiNotifier& notifier,
                      const ServerConfig& config, Scheduler& scheduler);

    void onNewConnection(int connectionId);
    void dispatch(int connectionId, const std::string& line);
    void onDisconnect(int connectionId);

    private:
    void _dispatchAi(int connectionId, const std::string& line);
    void _dispatchGui(int connectionId, const std::string& line);
    void _executeNext(int connectionId);

    void _handleMsz(int connectionId);
    void _handleBct(int connectionId, int x, int y);
    void _handleMct(int connectionId);
    void _handleTna(int connectionId);
    void _handlePpo(int connectionId, int playerId);
    void _handlePlv(int connectionId, int playerId);
    void _handlePin(int connectionId, int playerId);
    void _handleSgt(int connectionId);
    void _handleSst(int freq);

    ClientManager& _clients;
    World& _world;
    GuiNotifier& _notifier;
    Scheduler& _scheduler;
    const ServerConfig& _config;
    HandshakeHandler _handshakeHandler;
    int _freq;

    std::unordered_map<int, std::deque<Ai::Command>> _queues;
    std::unordered_map<int, bool> _hasActive;
};
