#pragma once

#include <chrono>
#include <functional>
#include <queue>

/// A callback and the time it should fire. Min-heap ordered by fireAt.
struct TimedEvent {
    std::chrono::steady_clock::time_point fireAt;
    std::function<void()> callback;
    bool operator>(const TimedEvent& other) const { return fireAt > other.fireAt; }
};

/**
 * @brief Priority queue of timed callbacks. Drives all game timing.
 *
 * Every action duration (movement, incantation, starvation...) is a scheduled
 * callback. The server feeds msUntilNext() directly to poll() so the process
 * sleeps exactly as long as needed: no busy-waiting.
 *
 * @see server/doc.md - "core/Scheduler" and "Main Loop" sections.
 */
class Scheduler {
    public:
    void schedule(std::chrono::milliseconds delay, std::function<void()> cb);
    void tick();

    /**
     * @brief Milliseconds until the next event. Pass directly to poll() as timeout.
     * Returns -1 if the queue is empty (poll blocks until I/O arrives).
     */
    int msUntilNext() const;

    /**
     * @brief Rescale all pending fire times by @p ratio.
     *
     * Called when the GUI sends `sst T` to change frequency. Pass oldFreq / newFreq.
     * O(n log n) heap rebuild -- acceptable since sst is infrequent.
     */
    void rescale(float ratio);

    private:
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<TimedEvent>> _queue;
};
