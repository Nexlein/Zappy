#pragma once

#include "core/Args.hpp"
#include "core/Scheduler.hpp"
#include "core/World.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/GuiNotifier.hpp"
#include "network/ClientManager.hpp"
#include "network/Listener.hpp"

class Server {
    public:
    Server(const ServerConfig& config);

    void run();

    private:
    void _scheduleRespawn();
    void _logStartup() const;

    static constexpr int RESPAWN_INTERVAL_MS = 20000;

    ServerConfig _config;
    Listener _listener;
    ClientManager _clients;
    World _world;
    GuiNotifier _notifier;
    Scheduler _scheduler;
    CommandDispatcher _dispatcher;
};