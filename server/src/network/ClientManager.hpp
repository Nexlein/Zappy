#pragma once

#include <poll.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "interfaces/INetworkObserver.hpp"
#include "network/Connection.hpp"
#include "network/Listener.hpp"

/// What one poll() cycle produced: new clients, dropped clients, and the
/// complete lines received this cycle (each paired with its connection id).
struct PollResult {
    std::vector<int> newConnections;
    std::vector<int> disconnectedIds;
    std::vector<std::pair<int, std::string>> lines;
};

/**
 * @brief Owns every client socket and runs the poll() loop.
 *
 * One poll() call watches all sockets at once: it accepts new clients, reads
 * the ready ones, splits incoming bytes into '\n' lines, and flushes pending
 * writes. Clients are addressed by a stable id, not the raw fd. Observers are
 * told about all traffic.
 */
class ClientManager {
    public:
    explicit ClientManager(Listener& listener);

    /// Run one poll cycle (blocks up to @p timeoutMs); returns this cycle's events.
    PollResult poll(int timeoutMs);
    /// Queue @p msg to be sent to a client (flushed during poll()).
    void send(int connectionId, const std::string& msg);
    /// Close a client and drop its connection.
    void disconnect(int connectionId);
    /// Access the underlying Connection (e.g. to set its type/playerId).
    Connection& getConnection(int connectionId);

    /// Subscribe an observer to connect/disconnect/line events.
    void addNetworkObserver(INetworkObserver* observer);

    private:
    void _acceptNew(PollResult& result);
    void _handleEvents(const pollfd& pfd, PollResult& result);
    void _rebuildPollFds();

    Listener& _listener;
    std::unordered_map<int, Connection> _connections;
    std::unordered_map<int, int> _fdToId;
    std::vector<pollfd> _pollFds;
    int _nextId = 0;
    std::vector<INetworkObserver*> _observers;
};
