#pragma once

#include <string>

class INetworkObserver {
    public:
    virtual ~INetworkObserver() = default;

    virtual void onClientConnected(int connectionId) = 0;
    virtual void onClientDisconnected(int connectionId) = 0;
    virtual void onLineReceived(int connectionId, const std::string& line) = 0;
    virtual void onLineSent(int connectionId, const std::string& line) = 0;
};