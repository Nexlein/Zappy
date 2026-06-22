#pragma once

#include "core/Args.hpp"
#include "core/Scheduler.hpp"
#include "core/World.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/GuiNotifier.hpp"
#include "logging/LogObserver.hpp"
#include "logging/Logger.hpp"
#include "network/ClientManager.hpp"
#include "network/Listener.hpp"

/**
 * @brief Owns every subsystem and runs the main game loop.
 *
 * run() loops forever: poll the sockets, pass new connections / lines /
 * disconnects to the CommandDispatcher, then let the Scheduler fire any timers
 * that are due (respawn, starvation, incantations...). Single thread.
 */
class Server {
    public:
    Server(const ServerConfig& config);

    /// Run the game loop until the process is stopped.
    void run();

    private:
    /// (Re)schedule the periodic world resource respawn.
    void _scheduleRespawn();
    void _logStartup() const;
    /// Freeze the world and log the winner banner. Runs once when a team wins.
    void _handleGameOver();

    /// Resources respawn every 20 time units (subject spec).
    static constexpr int RESPAWN_INTERVAL_MS = 20000;

    ServerConfig _config;
    Listener _listener;
    ClientManager _clients;
    World _world;
    GuiNotifier _notifier;
    Logger _logger;
    LogObserver _logObserver;
    Scheduler _scheduler;
    CommandDispatcher _dispatcher;
    bool _gameOverHandled = false;
};