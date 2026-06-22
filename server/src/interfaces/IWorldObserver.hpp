#pragma once

#include <string>
#include <vector>

#include "core/data/Orientation.hpp"
#include "core/data/Resources.hpp"

/**
 * @brief Observer hooks for game-state changes (Observer pattern).
 *
 * World fires one of these whenever something happens (a player moves, a
 * resource is taken, an incantation ends...). Observers react without World
 * depending on them. Each hook maps to one GUI message, noted as "-> xxx".
 */
class IWorldObserver {
    public:
    virtual ~IWorldObserver() = default;

    /// Player spawned (egg hatched or initial connection). -> pnw
    virtual void onPlayerAdded(int playerId, int x, int y, Orientation orientation, int level,
                               const std::string& teamName) = 0;
    /// Player changed tile and/or facing. -> ppo
    virtual void onPlayerMoved(int playerId, int nx, int ny, Orientation newOrientation) = 0;
    /// Player's carried resources changed. -> pin
    virtual void onPlayerInventoryChanged(int playerId, int x, int y, Resources inventory) = 0;
    /// Player left (death or disconnect). -> pdi
    virtual void onPlayerRemoved(int playerId) = 0;
    /// Player got pushed off its tile by an eject. -> pex
    virtual void onPlayerEjected(int playerId) = 0;
    /// Player shouted a broadcast message. -> pbc
    virtual void onBroadcast(int playerId, const std::string& message) = 0;
    /// Player picked a resource up off the tile. -> pgt (+ bct/pin)
    virtual void onResourceTaken(int playerId, ResourceType resourceType, int tileX, int tileY,
                                 Resources resources) = 0;
    /// Player dropped a resource onto the tile. -> pdr (+ bct/pin)
    virtual void onResourceDropped(int playerId, ResourceType resourceType, int tileX, int tileY,
                                   Resources resources) = 0;
    /// A player laid an egg via Fork. -> enw
    virtual void onEggLaid(int eggId, int playerId, int x, int y) = 0;
    /// An initial team egg placed at game start (custom). -> sse
    virtual void onInitialEggSpawned(int eggId, const std::string& teamName, int x, int y) = 0;
    /// An egg hatched into a connectable slot. -> ebo
    virtual void onEggHatched(int eggId) = 0;
    /// Incantation began on a tile. @p participantIds = players involved. -> pic
    virtual void onIncantationStart(int x, int y, int level,
                                    const std::vector<int>& participantIds) = 0;
    /// Incantation finished, @p success = leveled up or not. -> pie
    virtual void onIncantationEnd(int x, int y, bool success) = 0;
    /// Player reached a new level. -> plv
    virtual void onPlayerLevelUp(int playerId, int newLevel) = 0;
    /// A team won (6 players at max level). -> seg
    virtual void onGameEnd(const std::string& winningTeam) = 0;
    /// A tile's resource contents changed (e.g. respawn). -> bct
    virtual void onTileChanged(int x, int y, Resources resources) = 0;
};