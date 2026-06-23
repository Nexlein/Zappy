#include "Connection.hpp"

#include <sys/socket.h>
#include <unistd.h>

Connection::Connection(int fd) : _fd(fd) {}

Connection::~Connection()
{
    if (_fd >= 0) close(_fd);
}

void Connection::appendRead(std::string_view data) { _readBuf.append(data); }

std::optional<std::string> Connection::nextLine()
{
    size_t pos = _readBuf.find('\n');
    if (pos == std::string::npos) return std::nullopt;

    std::string line = _readBuf.substr(0, pos);
    _readBuf.erase(0, pos + 1);
    return line;
}

void Connection::queueWrite(const std::string& msg) { _writeBuf.append(msg); }

void Connection::flushWrite()
{
    if (_writeBuf.empty()) return;
    ssize_t sent = send(_fd, _writeBuf.data(), _writeBuf.size(), MSG_NOSIGNAL);
    if (sent > 0) _writeBuf.erase(0, static_cast<size_t>(sent));
}

int Connection::fd() const { return _fd; }
ClientType Connection::type() const { return _type; }
void Connection::setType(ClientType type) { _type = type; }
int Connection::playerId() const { return _playerId; }
void Connection::setPlayerId(int id) { _playerId = id; }
bool Connection::hasPendingWrite() const { return !_writeBuf.empty(); }