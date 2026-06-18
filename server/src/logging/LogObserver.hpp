#pragma once

#include <string>
#include <vector>

#include "interfaces/INetworkObserver.hpp"
#include "interfaces/IWorldObserver.hpp"
#include "logging/Logger.hpp"

/**
 * @brief Bridges the Observer hooks to the Logger.
 *
 * Registered on both World (IWorldObserver) and ClientManager (INetworkObserver),
 * so every world mutation and every raw protocol line is logged automatically,
 * without scattering log calls across the game logic.
 *
 * Raw I/O is logged at Debug; world events at Info.
 */
class LogObserver : public IWorldObserver, public INetworkObserver {
    public:
    explicit LogObserver(Logger& logger);

    // INetworkObserver
    void onClientConnected(int connectionId) override;
    void onClientDisconnected(int connectionId) override;
    void onLineReceived(int connectionId, const std::string& line) override;
    void onLineSent(int connectionId, const std::string& line) override;

    // IWorldObserver
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
    void onIncantationStart(int x, int y, int level,
                            const std::vector<int>& participantIds) override;
    void onIncantationEnd(int x, int y, bool success) override;
    void onPlayerLevelUp(int playerId, int newLevel) override;
    void onGameEnd(const std::string& winningTeam) override;
    void onTileChanged(int x, int y, Resources resources) override;

    private:
    Logger& _logger;
};
