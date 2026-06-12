#pragma once

#include <string>
#include <vector>

#include "core/data/Player.hpp"
#include "network/ClientManager.hpp"

class GuiNotifier {
    public:
    explicit GuiNotifier(ClientManager& clients);

    void addGui(int connectionId);
    void removeGui(int connectionId);

    void send(int connectionId, const std::string& msg);
    void broadcast(const std::string& msg);

    void onPlayerNew(const Player& p);
    void onPlayerDeath(int id);

    private:
    ClientManager& _clients;
    std::vector<int> _guiIds;
};
