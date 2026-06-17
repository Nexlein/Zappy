#include "network/ClientManager.hpp"

#include <sys/socket.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

ClientManager::ClientManager(Listener& listener) : _listener(listener) {}

void ClientManager::_rebuildPollFds()
{
    _pollFds.clear();
    _fdToId.clear();
    _pollFds.push_back({_listener.fd(), POLLIN, 0});
    for (auto& [id, conn] : _connections) {
        short events = POLLIN | (conn.hasPendingWrite() ? POLLOUT : 0);
        _pollFds.push_back({conn.fd(), events, 0});
        _fdToId[conn.fd()] = id;
    }
}

PollResult ClientManager::poll(int timeoutMs)
{
    _rebuildPollFds();

    int ret = ::poll(_pollFds.data(), static_cast<nfds_t>(_pollFds.size()), timeoutMs);
    while (ret < 0 && errno == EINTR)
        ret = ::poll(_pollFds.data(), static_cast<nfds_t>(_pollFds.size()), timeoutMs);
    if (ret < 0) throw std::runtime_error(std::string("poll() failed: ") + strerror(errno));

    PollResult result;
    if (_pollFds[0].revents & POLLIN) _acceptNew(result);
    for (size_t i = 1; i < _pollFds.size(); ++i) _handleEvents(_pollFds[i], result);
    return result;
}

void ClientManager::_acceptNew(PollResult& result)
{
    int fd = _listener.accept();
    if (fd < 0) return;
    int id = _nextId++;
    _connections.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                         std::forward_as_tuple(fd));
    result.newConnections.push_back(id);
}

void ClientManager::_handleEvents(const pollfd& pfd, PollResult& result)
{
    if (!pfd.revents) return;

    int id = _fdToId.at(pfd.fd);
    auto& conn = _connections.at(id);

    if (pfd.revents & POLLIN) {
        char buf[4096];
        ssize_t n = ::recv(pfd.fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            result.disconnectedIds.push_back(id);
            return;
        }
        conn.appendRead({buf, static_cast<size_t>(n)});
        while (auto line = conn.nextLine()) result.lines.emplace_back(id, std::move(*line));
    }
    if (pfd.revents & (POLLHUP | POLLERR)) {
        result.disconnectedIds.push_back(id);
        return;
    }
    if (pfd.revents & POLLOUT) conn.flushWrite();
}

void ClientManager::send(int connectionId, const std::string& msg)
{
    auto it = _connections.find(connectionId);
    if (it != _connections.end()) it->second.queueWrite(msg);
}

void ClientManager::disconnect(int connectionId)
{
    auto it = _connections.find(connectionId);
    if (it == _connections.end()) return;
    it->second.flushWrite();
    _connections.erase(it);
}

Connection& ClientManager::getConnection(int connectionId)
{
    auto it = _connections.find(connectionId);
    if (it == _connections.end())
        throw std::out_of_range("ClientManager::getConnection: unknown id");
    return it->second;
}
