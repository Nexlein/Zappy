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
    Server(ServerConfig& config);

    int run();

    private:
    ServerConfig& _config;
    Listener _listener;
    ClientManager _clients;
    World _world;
    GuiNotifier _notifier;
    Scheduler _scheduler;
    CommandDispatcher _dispatcher;
};