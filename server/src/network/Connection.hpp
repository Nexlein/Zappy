#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "ClientType.hpp"

class Connection {
    public:
    explicit Connection(int fd);
    ~Connection();

    void appendRead(std::string_view data);
    std::optional<std::string> nextLine();

    void queueWrite(const std::string& msg);
    void flushWrite();

    int fd() const;
    ClientType type() const;
    void setType(ClientType type);
    int playerId() const;
    void setPlayerId(int id);
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