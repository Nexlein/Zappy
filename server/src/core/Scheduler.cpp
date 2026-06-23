#include "Scheduler.hpp"

void Scheduler::schedule(std::chrono::milliseconds delay, std::function<void()> cb)
{
    _queue.push({std::chrono::steady_clock::now() + delay, std::move(cb)});
}

void Scheduler::tick()
{
    auto now = std::chrono::steady_clock::now();
    while (!_queue.empty() && _queue.top().fireAt <= now) {
        auto cb = std::move(_queue.top().callback);
        _queue.pop();
        cb();
    }
}

int Scheduler::msUntilNext() const
{
    if (_queue.empty()) return -1;

    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(_queue.top().fireAt - now);
    return diff.count() < 0 ? 0 : static_cast<int>(diff.count());
}

void Scheduler::clear()
{
    while (!_queue.empty()) _queue.pop();
}

void Scheduler::rescale(float ratio)
{
    auto now = std::chrono::steady_clock::now();
    std::vector<TimedEvent> events;
    while (!_queue.empty()) {
        events.push_back(_queue.top());
        _queue.pop();
    }
    for (auto& e : events) {
        auto remaining = e.fireAt - now;
        auto scaled = std::chrono::duration_cast<std::chrono::nanoseconds>(remaining * ratio);
        e.fireAt = now + scaled;
        _queue.push(std::move(e));
    }
}