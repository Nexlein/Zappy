#pragma once

#include "WorldState.hpp"
#include "Event.hpp"

class GameState {
public:
    WorldState world;
    int timeUnit = -1;
    std::string winnerTeam = "";

    void applyEvent(const Event& e);

    bool isDirty() const { return dirty; }
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
