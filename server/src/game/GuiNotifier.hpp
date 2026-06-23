#pragma once

#include <string>
#include <vector>

#include "interfaces/IWorldObserver.hpp"
#include "network/ClientManager.hpp"

class GuiNotifier : public IWorldObserver {
    public:
    explicit GuiNotifier(ClientManager& clients);

    void addGui(int connectionId);
    void removeGui(int connectionId);

    void send(int connectionId, const std::string& msg);
    void broadcast(const std::string& msg);

    void onPlayerAdded(int playerId, int x, int y, Orientation orientation, int level,
                       const std::string& teamName) override;
    void onPlayerMoved(int playerId, int nx, int ny, Orientation newOrientation) override;
    void onPlayerInventoryChanged(int playerId, int x, int y, Resources inventory) override;
    void onPlayerRemoved(int playerId) override;
    void onPlayerEjected(int playerId) override;
    void onBroadcast(int playerId, const std::string& message) override;
    void onResourceTaken(int playerId, ResourceType resourceType, int tileX, int tileY,
                         Resources resources) override;
    void onResourceDropped(int playerId, ResourceType resourceType, int tileX, int tileY,
                           Resources resources) override;
    void onEggLaid(int eggId, int playerId, int x, int y) override;
    void onInitialEggSpawned(int eggId, const std::string& teamName, int x, int y) override;
    void onEggHatched(int eggId) override;
    void onEggDied(int eggId) override;
    void onIncantationStart(int x, int y, int level,
                            const std::vector<int>& participantIds) override;
    void onIncantationEnd(int x, int y, bool success) override;
    void onPlayerLevelUp(int playerId, int newLevel) override;
    void onGameEnd(const std::string& winningTeam) override;
    void onTileChanged(int x, int y, Resources resources) override;

    private:
    ClientManager& _clients;
    std::vector<int> _guiIds;
};
