#pragma once

#include <string>

/**
 * @brief Observer hooks for raw socket events (Observer pattern).
 *
 * ClientManager calls these when a socket connects, drops, or moves a line, so
 * observers can react to traffic without ClientManager knowing about them.
 */
class INetworkObserver {
    public:
    virtual ~INetworkObserver() = default;

    /// A new client socket was accepted (assigned @p connectionId).
    virtual void onClientConnected(int connectionId) = 0;
    /// A client socket closed or errored.
    virtual void onClientDisconnected(int connectionId) = 0;
    /// One complete '\n'-terminated line arrived from the client.
    virtual void onLineReceived(int connectionId, const std::string& line) = 0;
    /// One line was queued to be sent to the client.
    virtual void onLineSent(int connectionId, const std::string& line) = 0;
};