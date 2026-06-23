#pragma once

#include <chrono>

/**
 * @brief Single owner of game frequency and elapsed game time.
 *
 * A tick is one time unit, lasting 1/freq seconds. The server is wall-clock
 * driven, so ticks are not counted one by one: instead this integrates freq
 * over time. Every freq change (GUI `sst`) banks the ticks accrued so far at
 * the old rate, so `ticks()` stays correct across any number of changes.
 *
 * @see server/doc.md - "core/GameClock"
 */
class GameClock {
    public:
    explicit GameClock(int freq);

    int freq() const { return _freq; }

    /// Wall-clock time since construction.
    std::chrono::microseconds elapsed() const;

    /// Total ticks elapsed: integral of freq over time. Fractional by nature.
    double ticks() const;

    /**
     * @brief Change frequency, banking ticks accrued at the old rate first.
     * @return oldFreq / newFreq, the ratio to feed Scheduler::rescale().
     */
    float setFreq(int newFreq);

    private:
    double _ticksSince(std::chrono::steady_clock::time_point from) const;

    int _freq;
    std::chrono::steady_clock::time_point _startTime;
    std::chrono::steady_clock::time_point _lastFreqChange;
    double _accumTicks = 0.0;
};
