#include "core/Server.hpp"

#include <iostream>
#include <memory>

#include "logging/CompositeSink.hpp"
#include "logging/ConsoleSink.hpp"
#include "logging/FileSink.hpp"

Server::Server(const ServerConfig& config)
    : _config(config),
      _listener(config.port),
      _clients(_listener),
      _world(config.width, config.height, config.teamNames),
      _notifier(_clients),
      _logObserver(_logger),
      _dispatcher(_clients, _world, _notifier, _config, _scheduler)
{
    auto sinks = std::make_unique<CompositeSink>();
    sinks->add(std::make_unique<ConsoleSink>());
    sinks->add(std::make_unique<FileSink>("zappy_server.log"));
    _logger.setSink(std::move(sinks));

    _world.addWorldObserver(&_notifier);
    _world.addWorldObserver(&_logObserver);
    _clients.addNetworkObserver(&_logObserver);

    _world.spawnInitialEggs(_config.clientsNb);
    _world.spawnResources();
}

void Server::_scheduleRespawn()
{
    _scheduler.schedule(std::chrono::milliseconds(RESPAWN_INTERVAL_MS / _config.freq), [this] {
        _world.spawnResources();
        _scheduleRespawn();
    });
}

void Server::_logStartup() const
{
    std::cout << "Server listening on port " << _config.port << "\n";
    std::cout << "Map size: " << _config.width << "x" << _config.height << "\n";
    std::cout << "Teams: ";
    for (const auto& name : _config.teamNames) std::cout << name << " ";
    std::cout << "\n";
}

void Server::run()
{
    _scheduleRespawn();
    _logStartup();

    while (true) {
        try {
            int timeout = _scheduler.msUntilNext();
            PollResult pr = _clients.poll(timeout);

            for (int id : pr.newConnections) _dispatcher.onNewConnection(id);
            for (auto& [connId, line] : pr.lines) _dispatcher.dispatch(connId, line);
            for (int id : pr.disconnectedIds) {
                _dispatcher.onDisconnect(id);
                _clients.disconnect(id);
            }

            _scheduler.tick();
            for (int id : _dispatcher.drainPendingDisconnects()) {
                _dispatcher.onDisconnect(id);
                _clients.disconnect(id);
            }
        } catch (const std::exception& e) {
            std::cerr << "[error] " << e.what() << "\n";
        }
    }
}
