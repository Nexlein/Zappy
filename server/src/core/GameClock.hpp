#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <unordered_map>

/**
 * @brief Single owner of game frequency and elapsed game time.
 *
 * A tick is one time unit, lasting 1/freq seconds. The server is wall-clock
 * driven, so ticks are not counted one by one: instead this integrates freq
 * over time. Every freq change (GUI `sst`) banks the ticks accrued so far at
 * the old rate, so `ticks()` stays correct across any number of changes.
 *
 * Also stamps when each team first joined, so win timing can be measured from
 * the team's entry rather than from server boot (which includes idle lobby
 * time before any AI connected).
 *
 * @see server/doc.md - "core/GameClock"
 */
class GameClock {
    public:
    /// A clock reading: wall time and ticks since boot, captured at one instant.
    struct Stamp {
        std::chrono::microseconds elapsed;
        double ticks;
    };

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

    /// Stamp a team's join moment. Write-once: later calls for the same team
    /// are ignored, so it keeps the first player's entry.
    void recordJoin(const std::string& team);

    /// The join stamp for @p team, or nullopt if the team never joined.
    std::optional<Stamp> joinOf(const std::string& team) const;

    private:
    double _ticksSince(std::chrono::steady_clock::time_point from) const;

    int _freq;
    std::chrono::steady_clock::time_point _startTime;
    std::chrono::steady_clock::time_point _lastFreqChange;
    double _accumTicks = 0.0;
    std::unordered_map<std::string, Stamp> _joins;
};
