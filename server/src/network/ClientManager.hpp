#pragma once

#include <poll.h>

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "network/Connection.hpp"
#include "network/Listener.hpp"

struct PollResult {
    std::vector<int> newFds;
    std::vector<std::pair<int, std::string>> lines;
};

class ClientManager {
    public:
    explicit ClientManager(Listener& listener);

    PollResult poll(int timeoutMs);
    void send(int connectionId, const std::string& msg);
    void disconnect(int connectionId);
    Connection& getConnection(int connectionId);

    private:
    void _acceptNew(PollResult& result);
    void _handleEvents(const pollfd& pfd, PollResult& result);
    void _rebuildPollFds();

    Listener& _listener;
    std::unordered_map<int, Connection> _connections;
    std::unordered_map<int, int> _fdToId;
    std::vector<pollfd> _pollFds;
    int _nextId = 0;
};
