#include "core/Server.hpp"

#include <chrono>
#include <cmath>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

    volatile std::sig_atomic_t g_stopRequested = 0;
    extern "C" void requestStop(int) { g_stopRequested = 1; }

}  // namespace

#include "logging/CompositeSink.hpp"
#include "logging/ConsoleSink.hpp"
#include "logging/FileSink.hpp"
#include "protocol/Serializer.hpp"

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
    // Both at Info: structural events only. Debug (per-tick NET I/O, broadcasts)
    // is dropped to keep the log bounded under broadcast-heavy AI traffic.
    sinks->add(std::make_unique<ConsoleSink>(LogLevel::Info));
    sinks->add(FileSink::forRun("server_p" + std::to_string(_config.port), LogLevel::Info));
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

void Server::_handleGameOver()
{
    _gameOverHandled = true;
    // Freeze the world: cancelling pending events stops moves, incantations,
    // respawns and starvation, so the final state stays visible on the GUI.
    _scheduler.clear();

    // Close every AI socket so the AI programs hit EOF and exit cleanly. We close
    // at the socket level WITHOUT removing the players from the world, so no pdi is
    // emitted and the GUI keeps showing the final board. GUI sockets stay open.
    std::vector<int> aiConns;
    for (const auto& [pid, p] : _world.getPlayers()) aiConns.push_back(p.connectionId);
    for (int conn : aiConns) _clients.disconnect(conn);

    long long upSeconds = _dispatcher.gameElapsed().count() / 1000000;
    long long upTicks = std::llround(_dispatcher.gameTicks());

    std::string teams;
    for (const auto& name : _config.teamNames) teams += (teams.empty() ? "" : " ") + name;
    const std::string winner = _world.winner().value_or("?");

    _logger.info("GAME", "========== GAME OVER ==========");
    _logger.info("GAME", "Winner: " + winner);
    _logger.info("GAME", "Teams: " + teams);
    _logger.info("GAME", "Server uptime: " + std::to_string(upSeconds) + " s (" +
                             std::to_string(upTicks) + " ticks)");

    // Time-to-win measured from when the winning team entered, not from server
    // boot (which would count idle lobby time before any AI connected).
    if (auto join = _dispatcher.teamJoin(winner)) {
        long long joinSeconds = join->elapsed.count() / 1000000;
        long long joinTicks = std::llround(join->ticks);
        _logger.info("GAME", winner + " joined at: " + std::to_string(joinSeconds) + " s (" +
                                 std::to_string(joinTicks) + " ticks)");
        _logger.info("GAME", winner + " took: " + std::to_string(upSeconds - joinSeconds) + " s (" +
                                 std::to_string(upTicks - joinTicks) + " ticks) to win");
        _notifier.broadcast(Serializer::gwt(winner, static_cast<int>(upSeconds - joinSeconds),
                                            upTicks - joinTicks));
    }
    _logger.info("GAME", "===============================");
}

void Server::run()
{
    std::signal(SIGINT, requestStop);
    std::signal(SIGTERM, requestStop);

    _scheduleRespawn();
    _logStartup();

    while (!g_stopRequested) {
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

            if (_world.isGameEnded() && !_gameOverHandled) _handleGameOver();
        } catch (const std::exception& e) {
            std::cerr << "[error] " << e.what() << "\n";
        }
    }

    _logger.info("Server", "Shutdown signal received, closing server");
}
