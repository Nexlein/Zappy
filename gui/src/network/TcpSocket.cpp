#include "TcpSocket.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

TcpSocket::~TcpSocket()
{
    if (_fd != -1) close(_fd);
}

void TcpSocket::connect(const std::string& host, int port)
{
    // Create socket
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == -1) {
        throw TcpException("Failed to create socket: " + std::string(strerror(errno)));
    }

    // Set non-blocking mode
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags == -1 || fcntl(_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(_fd);
        _fd = -1;
        throw TcpException("Failed to set non-blocking mode: " + std::string(strerror(errno)));
    }

    // Resolve hostname
    struct addrinfo hints {};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    int err = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
    if (err != 0) {
        close(_fd);
        _fd = -1;
        throw TcpException("Failed to resolve host: " + std::string(gai_strerror(err)));
    }

    // Try to connect (non-blocking will return immediately)
    int connectResult = ::connect(_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    if (connectResult == -1 && errno != EINPROGRESS) {
        close(_fd);
        _fd = -1;
        throw TcpException("Failed to connect: " + std::string(strerror(errno)));
    }

    // Wait for connection to complete (with timeout)
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(_fd, &writefds);

    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 second timeout
    timeout.tv_usec = 0;

    int selectResult = select(_fd + 1, nullptr, &writefds, nullptr, &timeout);
    if (selectResult <= 0) {
        close(_fd);
        _fd = -1;
        throw TcpException(selectResult == 0 ? "Connection timeout" : "Connection failed");
    }

    // Check if connection succeeded
    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1 || error != 0) {
        close(_fd);
        _fd = -1;
        throw TcpException("Connection failed: " + std::string(strerror(error)));
    }
}

void TcpSocket::send(const std::string& data)
{
    if (_fd == -1) {
        throw TcpException("Socket not connected");
    }

    ssize_t sent = ::send(_fd, data.c_str(), data.size(), 0);
    if (sent == -1) {
        throw TcpException("Failed to send data");
    }
}

std::optional<std::string> TcpSocket::recvLine()
{
    if (_fd == -1) {
        throw TcpException("Socket not connected");
    }

    // Check if we already have a complete line in the buffer
    size_t newlinePos = _recvBuffer.find('\n');
    if (newlinePos != std::string::npos) {
        std::string line = _recvBuffer.substr(0, newlinePos + 1);  // include \n
        _recvBuffer.erase(0, newlinePos + 1);
        return line;
    }

    // Try to read more data (non-blocking)
    char buffer[4096];
    ssize_t received = recv(_fd, buffer, sizeof(buffer), MSG_DONTWAIT);

    if (received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available right now, not an error
            return std::nullopt;
        }
        throw TcpException("Failed to receive data: " + std::string(strerror(errno)));
    } else if (received == 0) {
        // Connection closed
        return std::nullopt;
    }

    // Append new data to buffer
    _recvBuffer.append(buffer, received);

    // Check again for complete line after appending
    newlinePos = _recvBuffer.find('\n');
    if (newlinePos != std::string::npos) {
        std::string line = _recvBuffer.substr(0, newlinePos + 1);  // include \n
        _recvBuffer.erase(0, newlinePos + 1);
        return line;
    }

    // Still no complete line
    return std::nullopt;
}

bool TcpSocket::poll(int timeout_ms)
{
    if (_fd == -1) {
        throw TcpException("Socket not connected");
    }

    // If we have buffered data with a newline, return true immediately
    if (_recvBuffer.find('\n') != std::string::npos) {
        return true;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_fd, &readfds);

    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    int result = select(_fd + 1, &readfds, nullptr, nullptr, &timeout);
    if (result == -1) {
        throw TcpException("Failed to poll socket: " + std::string(strerror(errno)));
    }
    return result > 0;  // data is ready
}
