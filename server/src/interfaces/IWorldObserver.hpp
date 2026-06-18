#pragma once

#include <string>
#include <vector>

#include "core/data/Orientation.hpp"
#include "core/data/Resources.hpp"

class IWorldObserver {
    public:
    virtual ~IWorldObserver() = default;

    virtual void onPlayerAdded(int playerId, int x, int y, Orientation orientation, int level,
                               const std::string& teamName) = 0;
    virtual void onPlayerMoved(int playerId, int nx, int ny, Orientation newOrientation) = 0;
    virtual void onPlayerInventoryChanged(int playerId, int x, int y, Resources inventory) = 0;
    virtual void onPlayerRemoved(int playerId) = 0;
    virtual void onPlayerEjected(int playerId) = 0;
    virtual void onBroadcast(int playerId, const std::string& message) = 0;
    virtual void onResourceTaken(int playerId, ResourceType resourceType, int tileX, int tileY,
                                 Resources resources) = 0;
    virtual void onResourceDropped(int playerId, ResourceType resourceType, int tileX, int tileY,
                                   Resources resources) = 0;
    virtual void onEggLaid(int eggId, int playerId, int x, int y) = 0;
    virtual void onInitialEggSpawned(int eggId, const std::string& teamName, int x, int y) = 0;
    virtual void onEggHatched(int eggId) = 0;
    virtual void onIncantationStart(int x, int y, int level,
                                    const std::vector<int>& participantIds) = 0;
    virtual void onIncantationEnd(int x, int y, bool success) = 0;
    virtual void onPlayerLevelUp(int playerId, int newLevel) = 0;
    virtual void onGameEnd(const std::string& winningTeam) = 0;
    virtual void onTileChanged(int x, int y, Resources resources) = 0;
};