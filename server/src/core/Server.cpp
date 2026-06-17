#include "core/Server.hpp"

#include <iostream>

#include "protocol/Serializer.hpp"

Server::Server(const ServerConfig& config)
    : _config(config),
      _listener(config.port),
      _clients(_listener),
      _world(config.width, config.height, config.teamNames),
      _notifier(_clients),
      _dispatcher(_clients, _world, _notifier, _config, _scheduler)
{
    _world.spawnResources();
}

void Server::_scheduleRespawn()
{
    _scheduler.schedule(std::chrono::milliseconds(RESPAWN_INTERVAL_MS / _config.freq), [this] {
        auto changed = _world.spawnResources();
        for (auto [x, y] : changed)
            _notifier.broadcast(Serializer::bct(x, y, _world.at(x, y).resources));
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
        } catch (const std::exception& e) {
            std::cerr << "[error] " << e.what() << "\n";
        }
    }
}
