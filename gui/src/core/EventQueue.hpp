#pragma once

#include "Event.hpp"
#include <queue>
#include <mutex>
#include <optional>

/**
 * @brief Thread-safe queue for storing events received from the server.
 * Allows for potential multi-threaded behavior between GUI layers.
 * Not strictly necessary, good to have, future proof.
 */
class EventQueue {
public:
    /**
     * @brief Pushes an event to the queue.
     * Thread safe.
     * @param event The event to push.
     */
    void push(Event event);

    /**
     * @brief Pops an event from the queue.
     * Thread safe.
     * @return An optional containing the popped event, or std::nullopt if the queue is empty.
     */
    std::optional<Event> pop();

private:
    std::queue<Event> _queue;
    std::mutex _mutex;
};
