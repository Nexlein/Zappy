#include <chrono>
#include <cmath>
#include <stdexcept>

#include "CommandDispatcher.hpp"
#include "protocol/Serializer.hpp"

void CommandDispatcher::_handleMsz(int connectionId)
{
    _clients.send(connectionId, Serializer::msz(_world.width(), _world.height()));
}

void CommandDispatcher::_handleBct(int connectionId, int x, int y)
{
    _clients.send(connectionId, Serializer::bct(x, y, _world.at(x, y).resources));
}

void CommandDispatcher::_handleMct(int connectionId)
{
    for (int y = 0; y < _world.height(); ++y)
        for (int x = 0; x < _world.width(); ++x)
            _clients.send(connectionId, Serializer::bct(x, y, _world.at(x, y).resources));
}

void CommandDispatcher::_handleTna(int connectionId)
{
    for (const auto& team : _config.teamNames) _clients.send(connectionId, Serializer::tna(team));
}

void CommandDispatcher::_handlePpo(int connectionId, int playerId)
{
    try {
        auto& p = _world.getPlayer(playerId);
        _clients.send(connectionId, Serializer::ppo(p.id, p.x, p.y, p.orientation));
    } catch (const std::out_of_range&) {
        _clients.send(connectionId, "suc\n");
    }
}

void CommandDispatcher::_handlePlv(int connectionId, int playerId)
{
    try {
        auto& p = _world.getPlayer(playerId);
        _clients.send(connectionId, Serializer::plv(p.id, p.level));
    } catch (const std::out_of_range&) {
        _clients.send(connectionId, "suc\n");
    }
}

void CommandDispatcher::_handlePin(int connectionId, int playerId)
{
    try {
        auto& p = _world.getPlayer(playerId);
        _clients.send(connectionId, Serializer::pin(p.id, p.x, p.y, p.inventory));
    } catch (const std::out_of_range&) {
        _clients.send(connectionId, "suc\n");
    }
}

void CommandDispatcher::_handleSgt(int connectionId)
{
    _clients.send(connectionId, Serializer::sgt(_clock.freq()));
}

void CommandDispatcher::_handleStu(int connectionId)
{
    // GUI displays whole seconds: truncate the elapsed time to an integer second.
    // Ticks come alongside so the GUI can also show freq-accurate game time.
    int seconds =
        static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(gameElapsed()).count());
    long long ticks = std::llround(_clock.ticks());
    _clients.send(connectionId, Serializer::stu(seconds, ticks));
}

void CommandDispatcher::_handleGtt(int connectionId, const std::string& team)
{
    auto join = _clock.joinOf(team);
    if (!join) {
        _clients.send(connectionId, Serializer::gtt(team, -1, -1));
        return;
    }
    int seconds =
        static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(join->elapsed).count());
    long long ticks = std::llround(join->ticks);
    _clients.send(connectionId, Serializer::gtt(team, seconds, ticks));
}

void CommandDispatcher::_handleSst(int freq)
{
    float ratio = _clock.setFreq(freq);
    _scheduler.rescale(ratio);
    _notifier.broadcast(Serializer::sst(_clock.freq()));
}
