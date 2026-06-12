#include "Listener.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>

Listener::Listener(int port)
{
    if (port < 1 || port > 65535) throw ListenerException("invalid port: " + std::to_string(port));

    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0) throw ListenerException("socket() failed: " + std::string(strerror(errno)));

    int opt = 1;
    setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(_fd);
        throw ListenerException("bind() failed on port " + std::to_string(port) + ": " +
                                strerror(errno));
    }

    if (listen(_fd, SOMAXCONN) < 0) {
        close(_fd);
        throw ListenerException("listen() failed: " + std::string(strerror(errno)));
    }
}

Listener::~Listener()
{
    if (_fd >= 0) close(_fd);
}

int Listener::fd() const { return _fd; }

int Listener::accept() const
{
    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);
    return ::accept(_fd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
}
