#pragma once

#include <memory>

#include "Event.hpp"
#include "VisualState.hpp"
#include "WorldState.hpp"
#include "behaviors/IBehavior.hpp"

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
    int gameDurationSeconds = -1;  // gwt: time the winner took to win (-1 = not yet received)
    int64_t gameDurationTicks = -1;  // gwt: same in game-time ticks (-1 = not yet received)
    float tileSize = 1.0f;
    unsigned int serverUptimeSeconds = 0;
    bool receivedStuResponse = false;

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

    void _applyMapSize(const MapSize& e);
    void _applyTileContent(const TileContent& e);
    void _applyTeamName(const TeamName& e);
    void _applyPlayerNew(const PlayerNew& e);
    void _applyPlayerPosition(const PlayerPosition& e);
    void _applyPlayerLevel(const PlayerLevel& e);
    void _applyPlayerInventory(const PlayerInventory& e);
    void _applyPlayerExpulsion(const PlayerExpulsion& e);
    void _applyPlayerBroadcast(const PlayerBroadcast& e);
    void _applyIncantationStart(const IncantationStart& e);
    void _applyIncantationEnd(const IncantationEnd& e);
    void _applyPlayerFork(const PlayerFork& e);
    void _applyPlayerResourceDrop(const PlayerResourceDrop& e);
    void _applyPlayerResourceTake(const PlayerResourceTake& e);
    void _applyPlayerDeath(const PlayerDeath& e);
    void _applyEggNew(const EggNew& e);
    void _applyEggHatch(const EggHatch& e);
    void _applyEggDeath(const EggDeath& e);
    void _applyTimeUnit(const TimeUnit& e);
    void _applyTimeUnitChange(const TimeUnitChange& e);
    void _applyGameEnd(const GameEnd& e);
    void _applyServerUptime(const ServerUptime& e);
    void _applyServerSpawnedEgg(const ServerSpawnedEgg& e);
    void _applyWinDuration(const WinDuration& e);

    static void _pushBehavior(VisualState& visual, std::unique_ptr<IBehavior> b);
};
