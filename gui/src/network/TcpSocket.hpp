#pragma once

#include <optional>
#include <string>
#include <exception>

/**
 * @brief Exception class for TCP errors.
 */
class TcpException : public std::exception {
public:
    TcpException(const std::string& message) : _message(message) {}
    const char* what() const noexcept override { return _message.c_str(); }

private:
    std::string _message;
};

/**
 * @brief Simple TCP socket wrapper.
 */
class TcpSocket {
public:
    /**
     * @brief Wraps close().
     */
    ~TcpSocket();

    /**
     * @brief Connects to a remote host.
     * @param host The hostname or IP address to connect to.
     * @param port The port to connect to.
     * @throws TcpException on failure.
     */
    void connect(const std::string& host, int port);

    /**
     * @brief Sends data through the socket.
     * @param data The data to send.
     * @throws TcpException on failure.
     */
    void send(const std::string& data);

    /**
     * @brief Receives a line of text from the socket.
     * @return A complete line of text, or std::nullopt if the connection is closed.
     * @throws TcpException on failure.
     */
    std::optional<std::string> recvLine();

    /**
     * @brief Polls the socket for incoming data.
     * @param timeout_ms The timeout in milliseconds.
     * @return true if data is ready to be read, false if timeout occurred.
     * @throws TcpException on failure.
     */
    bool poll(int timeout_ms);

private:
    int _fd = -1;
    std::string _recvBuffer;
};
