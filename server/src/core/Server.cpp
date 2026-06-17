#include "core/Server.hpp"
#include <iostream>
#include "protocol/Serializer.hpp"

Server::Server(ServerConfig& config)
    : _config(config),
      _listener(config.port),
      _clients(_listener),
      _world(config.width, config.height, config.teamNames),
      _notifier(_clients),
      _dispatcher(_clients, _world, _notifier, config, _scheduler)
{
    _world.spawnResources();
}

int Server::run()
{
    std::function<void()> scheduleRespawn;
    scheduleRespawn = [&] {
        _scheduler.schedule(std::chrono::milliseconds(20000 / _config.freq), [&] {
            auto changed = _world.spawnResources();
            for (auto [x, y] : changed)
                _notifier.broadcast(Serializer::bct(x, y, _world.at(x, y).resources));
            scheduleRespawn();
        });
    };
    scheduleRespawn();

    std::cout << "Server listening on port " << _config.port << "\n";
    std::cout << "Map size: " << _config.width << "x" << _config.height << "\n";
    std::cout << "Teams: ";
    for (const auto& name : _config.teamNames) std::cout << name << " ";
    std::cout << "\n";

    while (true) {
        int timeout = _scheduler.msUntilNext();
        PollResult pr = _clients.poll(timeout);

        for (int id : pr.disconnectedIds) {
            _dispatcher.onDisconnect(id);
            _clients.disconnect(id);
        }
        for (int id : pr.newConnections) _dispatcher.onNewConnection(id);
        for (auto& [connId, line] : pr.lines) {
            _dispatcher.dispatch(connId, line);
        }

        _scheduler.tick();
    }

}