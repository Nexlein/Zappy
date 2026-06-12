#pragma once

#include <chrono>
#include <functional>
#include <queue>
#include <thread>

struct TimedEvent {
    std::chrono::steady_clock::time_point fireAt;
    std::function<void()> callback;
    bool operator>(const TimedEvent& other) const { return fireAt > other.fireAt; }
};

class Scheduler {
    public:
    void schedule(std::chrono::milliseconds delay, std::function<void()> cb);
    void tick();
    int msUntilNext() const;
    void rescale(float ratio);

    private:
    std::priority_queue<TimedEvent, std::vector<TimedEvent>, std::greater<TimedEvent>> _queue;
};