#include "EventQueue.hpp"

void EventQueue::push(Event event)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push(std::move(event));
}

std::optional<Event> EventQueue::pop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_queue.empty()) return std::nullopt;

    Event event = std::move(_queue.front());
    _queue.pop();
    return event;
}

void EventQueue::clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _queue = {};
}
