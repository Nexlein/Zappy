#include "GameClock.hpp"

GameClock::GameClock(int freq)
    : _freq(freq),
      _startTime(std::chrono::steady_clock::now()),
      _lastFreqChange(_startTime)
{
}

std::chrono::microseconds GameClock::elapsed() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                                                 _startTime);
}

double GameClock::_ticksSince(std::chrono::steady_clock::time_point from) const
{
    auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - from).count();
    return seconds * _freq;
}

double GameClock::ticks() const
{
    return _accumTicks + _ticksSince(_lastFreqChange);
}

float GameClock::setFreq(int newFreq)
{
    _accumTicks += _ticksSince(_lastFreqChange);
    _lastFreqChange = std::chrono::steady_clock::now();
    float ratio = static_cast<float>(_freq) / newFreq;
    _freq = newFreq;
    return ratio;
}

void GameClock::recordJoin(const std::string& team)
{
    if (_joins.count(team)) return;
    _joins.emplace(team, Stamp{elapsed(), ticks()});
}

std::optional<GameClock::Stamp> GameClock::joinOf(const std::string& team) const
{
    auto it = _joins.find(team);
    if (it == _joins.end()) return std::nullopt;
    return it->second;
}
