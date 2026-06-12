#pragma once

#include "Event.hpp"
#include "WorldState.hpp"

/**
 * @brief Represents the current state of the game.
 * Knows the world state, some metadata, and how to apply events to update itself.
 * Also tracks whether it has been modified since the last time it was cleared (dirty flag).
 */
class GameState {
    public:
    WorldState world;
    int timeUnit = -1;
    std::string winnerTeam = "";
    float tileSize = 1.0f;

    /**
     * @brief Applies an event to the game state, modifying it accordingly and setting the dirty
     * flag.
     */
    void applyEvent(const Event& e);

    /**
     * @brief Checks if the game state has been modified since the last time it was cleared (dirty
     * flag).
     * @return true if the game state is dirty, false otherwise.
     */
    bool isDirty() const { return dirty; }

    /**
     * @brief Sets the dirty flag to false.
     * @note This should be called after the game state has been rendered or processed, to indicate
     * that it is now clean.
     */
    void clearDirty() const { dirty = false; }

    private:
    mutable bool dirty = false;

    void applyMapSize(const MapSize& e);
    void applyTileContent(const TileContent& e);
    void applyTeamName(const TeamName& e);
    void applyPlayerNew(const PlayerNew& e);
    void applyPlayerPosition(const PlayerPosition& e);
    void applyPlayerLevel(const PlayerLevel& e);
    void applyPlayerInventory(const PlayerInventory& e);
    void applyPlayerExpulsion(const PlayerExpulsion& e);
    void applyPlayerBroadcast(const PlayerBroadcast& e);
    void applyIncantationStart(const IncantationStart& e);
    void applyIncantationEnd(const IncantationEnd& e);
    void applyPlayerFork(const PlayerFork& e);
    void applyPlayerResourceDrop(const PlayerResourceDrop& e);
    void applyPlayerResourceTake(const PlayerResourceTake& e);
    void applyPlayerDeath(const PlayerDeath& e);
    void applyEggNew(const EggNew& e);
    void applyEggHatch(const EggHatch& e);
    void applyEggDeath(const EggDeath& e);
    void applyTimeUnit(const TimeUnit& e);
    void applyTimeUnitChange(const TimeUnitChange& e);
    void applyGameEnd(const GameEnd& e);
};
