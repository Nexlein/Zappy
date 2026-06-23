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
