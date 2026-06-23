#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "ClientType.hpp"

/**
 * @brief One client socket plus its read/write buffers.
 *
 * Owns the fd (closed on destruction). Incoming bytes go into the read buffer,
 * then come out one '\n' line at a time; outgoing data is queued and flushed
 * when the socket is writable. Also tracks the client kind (PENDING/AI/GUI)
 * and, for an AI, which player it controls. Non-copyable.
 */
class Connection {
    public:
    explicit Connection(int fd);
    ~Connection();

    /// Append freshly-read bytes to the read buffer.
    void appendRead(std::string_view data);
    /// Pop one complete '\n'-terminated line, or nullopt if none buffered yet.
    std::optional<std::string> nextLine();

    /// Append @p msg to the write buffer (not sent until flushWrite()).
    void queueWrite(const std::string& msg);
    /// Try to send the buffered write data over the socket.
    void flushWrite();

    int fd() const;
    /// Client kind: PENDING until the handshake promotes it to AI or GUI.
    ClientType type() const;
    void setType(ClientType type);
    /// Player this connection drives (-1 for GUI / not yet assigned).
    int playerId() const;
    void setPlayerId(int id);
    /// True if there is still unsent data in the write buffer.
    bool hasPendingWrite() const;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    private:
    int _fd;
    std::string _readBuf;
    std::string _writeBuf;
    ClientType _type = ClientType::PENDING;
    int _playerId = -1;
};