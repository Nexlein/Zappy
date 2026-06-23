#pragma once

#include <exception>
#include <string>

class ListenerException : public std::exception {
    public:
    /**
     * @brief Construct exception with error message
     * @param message Description of the error
     */
    explicit ListenerException(const std::string& message) : _message(message) {}

    /**
     * @brief Get the error message
     * @return C-string containing the error description
     */
    const char* what() const noexcept override { return _message.c_str(); }

    private:
    std::string _message;
};

/**
 * @brief The server's listening socket.
 *
 * Binds and listens on a port in the constructor (throws on failure), closes
 * on destruction. Its fd is polled with the client sockets; when it's readable
 * a new client is waiting and accept() pulls it in. Non-copyable.
 */
class Listener {
    public:
    /// Bind and listen on @p port. @throws ListenerException on failure.
    explicit Listener(int port);
    ~Listener();

    /// The listening socket fd (to register in poll()).
    int fd() const;
    /// Accept one pending client; returns its new socket fd.
    int accept() const;

    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

    private:
    int _fd;
};