#include <gtest/gtest.h>

#include "../../server/src/core/World.hpp"

static World makeWorld(int w, int h) { return World(w, h, {"TeamA", "TeamB"}); }

// --- at() toroidal wrap ---

TEST(WorldAt, NormalCoords)
{
    auto w = makeWorld(10, 10);
    EXPECT_NO_THROW(w.at(0, 0));
    EXPECT_NO_THROW(w.at(9, 9));
}

TEST(WorldAt, WrapX)
{
    auto w = makeWorld(10, 10);
    EXPECT_EQ(&w.at(10, 0), &w.at(0, 0));
    EXPECT_EQ(&w.at(20, 0), &w.at(0, 0));
}

TEST(WorldAt, WrapY)
{
    auto w = makeWorld(10, 10);
    EXPECT_EQ(&w.at(0, 10), &w.at(0, 0));
    EXPECT_EQ(&w.at(0, 20), &w.at(0, 0));
}

TEST(WorldAt, NegativeX)
{
    auto w = makeWorld(10, 10);
    EXPECT_EQ(&w.at(-1, 0), &w.at(9, 0));
    EXPECT_EQ(&w.at(-10, 0), &w.at(0, 0));
}

TEST(WorldAt, NegativeY)
{
    auto w = makeWorld(10, 10);
    EXPECT_EQ(&w.at(0, -1), &w.at(0, 9));
    EXPECT_EQ(&w.at(0, -10), &w.at(0, 0));
}

// --- spawnResources() ---

TEST(WorldSpawn, TotalReachesTarget)
{
    auto w = makeWorld(10, 10);
    w.spawnResources();

    for (int i = 0; i < Resources::TYPE_COUNT; i++) {
        auto type = static_cast<ResourceType>(i);
        int target = std::max(1, (int)(10 * 10 * Resources::density(type)));

        int total = 0;
        for (int x = 0; x < 10; x++)
            for (int y = 0; y < 10; y++) total += w.at(x, y).resources[type];

        EXPECT_EQ(total, target) << "Resource type " << i << " mismatch";
    }
}

TEST(WorldSpawn, NeverBelowOne)
{
    auto w = makeWorld(2, 2);
    w.spawnResources();

    for (int i = 0; i < Resources::TYPE_COUNT; i++) {
        auto type = static_cast<ResourceType>(i);

        int total = 0;
        for (int x = 0; x < 2; x++)
            for (int y = 0; y < 2; y++) total += w.at(x, y).resources[type];

        EXPECT_GE(total, 1) << "Resource type " << i << " is zero";
    }
}

TEST(WorldSpawn, IdempotentWhenFull)
{
    auto w = makeWorld(10, 10);
    w.spawnResources();

    // count after first spawn
    int before = 0;
    for (int x = 0; x < 10; x++)
        for (int y = 0; y < 10; y++)
            before += w.at(x, y).resources[static_cast<ResourceType>(1)];  // linemate

    w.spawnResources();

    int after = 0;
    for (int x = 0; x < 10; x++)
        for (int y = 0; y < 10; y++) after += w.at(x, y).resources[static_cast<ResourceType>(1)];

    EXPECT_EQ(before, after);
}

// --- addPlayer() ---

TEST(WorldPlayer, AddPlayerReturnsIncrementingIds)
{
    auto w = makeWorld(10, 10);
    int id0 = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    int id1 = w.addPlayer(1, "TeamA", 0, 0, Orientation::N);
    EXPECT_EQ(id0, 0);
    EXPECT_EQ(id1, 1);
}

TEST(WorldPlayer, AddPlayerAppearsOnTile)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 3, 4, Orientation::N);
    auto& tile = w.at(3, 4);
    EXPECT_EQ(tile.playerIds.size(), 1u);
    EXPECT_EQ(tile.playerIds[0], id);
}

TEST(WorldPlayer, AddMultiplePlayersSameTile)
{
    auto w = makeWorld(10, 10);
    int id0 = w.addPlayer(0, "TeamA", 5, 5, Orientation::N);
    int id1 = w.addPlayer(1, "TeamB", 5, 5, Orientation::S);
    auto& tile = w.at(5, 5);
    EXPECT_EQ(tile.playerIds.size(), 2u);
    EXPECT_NE(id0, id1);
}

// --- removePlayer() ---

TEST(WorldPlayer, RemovePlayerClearsFromTile)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 1, 1, Orientation::N);
    w.removePlayer(id);
    EXPECT_EQ(w.at(1, 1).playerIds.size(), 0u);
}

TEST(WorldPlayer, RemoveOneOfMultiplePlayers)
{
    auto w = makeWorld(10, 10);
    int id0 = w.addPlayer(0, "TeamA", 2, 2, Orientation::N);
    int id1 = w.addPlayer(1, "TeamB", 2, 2, Orientation::N);
    w.removePlayer(id0);
    auto& tile = w.at(2, 2);
    EXPECT_EQ(tile.playerIds.size(), 1u);
    EXPECT_EQ(tile.playerIds[0], id1);
}

// --- movePlayer() ---

TEST(WorldPlayer, MovePlayerUpdatesPosition)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.movePlayer(id, 2, 3);
    EXPECT_EQ(w.at(0, 0).playerIds.size(), 0u);
    EXPECT_EQ(w.at(2, 3).playerIds.size(), 1u);
    EXPECT_EQ(w.at(2, 3).playerIds[0], id);
}

TEST(WorldPlayer, MovePlayerToroidalWrap)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 9, 9, Orientation::N);
    w.movePlayer(id, 10, 10);
    EXPECT_EQ(w.at(0, 0).playerIds.size(), 1u);
    EXPECT_EQ(w.at(9, 9).playerIds.size(), 0u);
}

// --- takeResource() / setResource() ---

TEST(WorldInventory, TakeResourceMovesFromTileToPlayer)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.at(0, 0).resources[ResourceType::LINEMATE] = 3;

    bool ok = w.takeResource(id, ResourceType::LINEMATE);

    EXPECT_TRUE(ok);
    EXPECT_EQ(w.at(0, 0).resources[ResourceType::LINEMATE], 2);
    EXPECT_EQ(w.getPlayer(id).inventory[ResourceType::LINEMATE], 1);
}

TEST(WorldInventory, TakeResourceFailsWhenTileEmpty)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);

    bool ok = w.takeResource(id, ResourceType::THYSTAME);

    EXPECT_FALSE(ok);
    EXPECT_EQ(w.getPlayer(id).inventory[ResourceType::THYSTAME], 0);
}

TEST(WorldInventory, SetResourceMovesFromPlayerToTile)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 2, 2, Orientation::N);
    w.at(2, 2).resources[ResourceType::FOOD] = 1;
    w.takeResource(id, ResourceType::FOOD);

    bool ok = w.setResource(id, ResourceType::FOOD);

    EXPECT_TRUE(ok);
    EXPECT_EQ(w.getPlayer(id).inventory[ResourceType::FOOD], 10);
    EXPECT_EQ(w.at(2, 2).resources[ResourceType::FOOD], 1);
}

TEST(WorldInventory, SetResourceFailsWhenInventoryEmpty)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);

    bool ok = w.setResource(id, ResourceType::SIBUR);

    EXPECT_FALSE(ok);
}

// --- ejectPlayers() ---

TEST(WorldEject, EjectMovesOtherPlayers)
{
    auto w = makeWorld(10, 10);
    int ejector = w.addPlayer(0, "TeamA", 5, 5, Orientation::N);
    int victim = w.addPlayer(1, "TeamB", 5, 5, Orientation::S);

    auto result = w.ejectPlayers(ejector);

    EXPECT_EQ(result.ejectedPlayerIds.size(), 1u);
    EXPECT_EQ(result.ejectedPlayerIds[0], victim);
    EXPECT_EQ(w.at(5, 5).playerIds.size(), 1u);  // only ejector remains
    EXPECT_EQ(w.getPlayer(victim).y, 4);         // N = y-1
}

TEST(WorldEject, EjectorStaysOnTile)
{
    auto w = makeWorld(10, 10);
    int ejector = w.addPlayer(0, "TeamA", 3, 3, Orientation::E);
    w.addPlayer(1, "TeamB", 3, 3, Orientation::N);

    w.ejectPlayers(ejector);

    EXPECT_EQ(w.getPlayer(ejector).x, 3);
    EXPECT_EQ(w.getPlayer(ejector).y, 3);
}

TEST(WorldEject, EjectDestroysEggsOnTile)
{
    auto w = makeWorld(10, 10);
    int ejector = w.addPlayer(0, "TeamA", 2, 2, Orientation::N);
    w.addEgg(ejector);

    w.ejectPlayers(ejector);

    EXPECT_EQ(w.at(2, 2).eggIds.size(), 0u);
}

TEST(WorldEject, EjectToroidalWrap)
{
    auto w = makeWorld(10, 10);
    int ejector = w.addPlayer(0, "TeamA", 5, 0, Orientation::N);
    int victim = w.addPlayer(1, "TeamB", 5, 0, Orientation::S);

    w.ejectPlayers(ejector);

    EXPECT_EQ(w.getPlayer(victim).y, 9);  // 0 - 1 wraps to 9
}

// --- addEgg() / hatchEgg() ---

TEST(WorldEgg, AddEggAppearsOnTile)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 3, 3, Orientation::N);
    int eid = w.addEgg(pid);

    EXPECT_EQ(w.at(3, 3).eggIds.size(), 1u);
    EXPECT_EQ(w.at(3, 3).eggIds[0], eid);
}

TEST(WorldEgg, HatchEggRemovesFromTile)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 1, 1, Orientation::N);
    int eid = w.addEgg(pid);

    bool ok = w.hatchEgg(eid);

    EXPECT_TRUE(ok);
    EXPECT_EQ(w.at(1, 1).eggIds.size(), 0u);
}

TEST(WorldEgg, HatchUnknownEggReturnsFalse)
{
    auto w = makeWorld(10, 10);

    EXPECT_FALSE(w.hatchEgg(999));
}

// --- incantation ---

TEST(Incantation, StartLevel1Success)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 5, 5, Orientation::E);
    w.at(5, 5).resources[ResourceType::LINEMATE] = 1;

    auto result = w.startIncantation(pid);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 1u);
    EXPECT_TRUE(w.getPlayer(pid).isIncanting);
}

TEST(Incantation, StartFailsMissingStones)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 3, 3, Orientation::N);
    // no linemate on tile

    auto result = w.startIncantation(pid);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(w.getPlayer(pid).isIncanting);
}

TEST(Incantation, StartFailsNotEnoughPlayers)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 3, 3, Orientation::N);
    w.getPlayer(pid).level = 2;
    // level 2 requires 2 players — only 1 here
    w.at(3, 3).resources[ResourceType::LINEMATE] = 1;
    w.at(3, 3).resources[ResourceType::DERAUMERE] = 1;
    w.at(3, 3).resources[ResourceType::SIBUR] = 1;

    auto result = w.startIncantation(pid);

    EXPECT_FALSE(result.has_value());
}

TEST(Incantation, StartLevel2TwoPlayersSuccess)
{
    auto w = makeWorld(10, 10);
    int p1 = w.addPlayer(0, "TeamA", 4, 4, Orientation::N);
    int p2 = w.addPlayer(1, "TeamB", 4, 4, Orientation::S);
    w.getPlayer(p1).level = 2;
    w.getPlayer(p2).level = 2;
    w.at(4, 4).resources[ResourceType::LINEMATE] = 1;
    w.at(4, 4).resources[ResourceType::DERAUMERE] = 1;
    w.at(4, 4).resources[ResourceType::SIBUR] = 1;

    auto result = w.startIncantation(p1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2u);
    EXPECT_TRUE(w.getPlayer(p1).isIncanting);
    EXPECT_TRUE(w.getPlayer(p2).isIncanting);
}

TEST(Incantation, StartIgnoresWrongLevelPlayers)
{
    auto w = makeWorld(10, 10);
    int p1 = w.addPlayer(0, "TeamA", 2, 2, Orientation::N);
    int p2 = w.addPlayer(1, "TeamB", 2, 2, Orientation::N);
    // p1 level 1, p2 level 2 — only p1 counts for level 1 ritual
    w.getPlayer(p2).level = 2;
    w.at(2, 2).resources[ResourceType::LINEMATE] = 1;

    auto result = w.startIncantation(p1);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 1u);  // only p1 in participants
    EXPECT_FALSE(w.getPlayer(p2).isIncanting);
}

TEST(Incantation, FinalizeSuccessLevelsUpAndConsumesStones)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.at(0, 0).resources[ResourceType::LINEMATE] = 1;

    auto participants = w.startIncantation(pid);
    ASSERT_TRUE(participants.has_value());

    bool ok = w.finalizeIncantation(0, 0, *participants);

    EXPECT_TRUE(ok);
    EXPECT_EQ(w.getPlayer(pid).level, 2);
    EXPECT_EQ(w.at(0, 0).resources[ResourceType::LINEMATE], 0);
    EXPECT_FALSE(w.getPlayer(pid).isIncanting);
}

TEST(Incantation, FinalizeFailsIfPlayerDied)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.at(0, 0).resources[ResourceType::LINEMATE] = 1;

    auto participants = w.startIncantation(pid);
    ASSERT_TRUE(participants.has_value());

    w.removePlayer(pid);  // player dies mid-ritual
    bool ok = w.finalizeIncantation(0, 0, *participants);

    EXPECT_FALSE(ok);
}

TEST(Incantation, FinalizeStonesConsumedMatchLevel)
{
    auto w = makeWorld(10, 10);
    int p1 = w.addPlayer(0, "TeamA", 1, 1, Orientation::N);
    int p2 = w.addPlayer(1, "TeamB", 1, 1, Orientation::N);
    w.getPlayer(p1).level = 2;
    w.getPlayer(p2).level = 2;
    w.at(1, 1).resources[ResourceType::LINEMATE] = 1;
    w.at(1, 1).resources[ResourceType::DERAUMERE] = 1;
    w.at(1, 1).resources[ResourceType::SIBUR] = 1;

    auto participants = w.startIncantation(p1);
    ASSERT_TRUE(participants.has_value());
    w.finalizeIncantation(1, 1, *participants);

    EXPECT_EQ(w.at(1, 1).resources[ResourceType::LINEMATE], 0);
    EXPECT_EQ(w.at(1, 1).resources[ResourceType::DERAUMERE], 0);
    EXPECT_EQ(w.at(1, 1).resources[ResourceType::SIBUR], 0);
    EXPECT_EQ(w.getPlayer(p1).level, 3);
    EXPECT_EQ(w.getPlayer(p2).level, 3);
}

TEST(Incantation, CheckWinNoWinnerYet)
{
    auto w = makeWorld(10, 10);
    w.addPlayer(0, "TeamA", 0, 0, Orientation::N);

    EXPECT_FALSE(w.checkWin().has_value());
}

TEST(Incantation, CheckWinDetectsWinner)
{
    auto w = makeWorld(10, 10);
    for (int i = 0; i < 6; i++) {
        int pid = w.addPlayer(i, "TeamA", i, 0, Orientation::N);
        w.getPlayer(pid).level = 8;
    }

    auto winner = w.checkWin();

    ASSERT_TRUE(winner.has_value());
    EXPECT_EQ(*winner, "TeamA");
}

TEST(Incantation, CheckWinRequiresSixPlayers)
{
    auto w = makeWorld(10, 10);
    for (int i = 0; i < 5; i++) {
        int pid = w.addPlayer(i, "TeamA", i, 0, Orientation::N);
        w.getPlayer(pid).level = 8;
    }

    EXPECT_FALSE(w.checkWin().has_value());
}

// --- popEggForTeam() ---

TEST(WorldEgg, PopEggForTeamReturnsMatchingEgg)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 2, 2, Orientation::N);
    w.addEgg(pid);

    auto egg = w.popEggForTeam("TeamA");

    ASSERT_TRUE(egg.has_value());
    EXPECT_EQ(egg->teamName, "TeamA");
}

TEST(WorldEgg, PopEggForTeamRemovesItFromWorld)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 2, 2, Orientation::N);
    w.addEgg(pid);

    w.popEggForTeam("TeamA");

    EXPECT_FALSE(w.popEggForTeam("TeamA").has_value());
}

TEST(WorldEgg, PopEggForTeamRemovesItFromTile)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 3, 3, Orientation::N);
    w.addEgg(pid);

    w.popEggForTeam("TeamA");

    EXPECT_EQ(w.at(3, 3).eggIds.size(), 0u);
}

TEST(WorldEgg, PopEggForTeamReturnsNulloptWhenNone)
{
    auto w = makeWorld(10, 10);

    EXPECT_FALSE(w.popEggForTeam("TeamA").has_value());
}

TEST(WorldEgg, PopEggForTeamIgnoresOtherTeams)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamB", 1, 1, Orientation::N);
    w.addEgg(pid);

    EXPECT_FALSE(w.popEggForTeam("TeamA").has_value());
    // TeamB egg is still there
    EXPECT_TRUE(w.popEggForTeam("TeamB").has_value());
}

TEST(WorldEgg, PopEggForTeamPopsOnlyOne)
{
    auto w = makeWorld(10, 10);
    int pid = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.addEgg(pid);
    w.addEgg(pid);

    w.popEggForTeam("TeamA");

    EXPECT_TRUE(w.popEggForTeam("TeamA").has_value());
}

// --- teamPlayerCount() ---

TEST(WorldTeam, TeamPlayerCountZeroWhenEmpty)
{
    auto w = makeWorld(10, 10);

    EXPECT_EQ(w.teamPlayerCount("TeamA"), 0);
}

TEST(WorldTeam, TeamPlayerCountIncrementsOnAdd)
{
    auto w = makeWorld(10, 10);
    w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.addPlayer(1, "TeamA", 1, 0, Orientation::N);

    EXPECT_EQ(w.teamPlayerCount("TeamA"), 2);
}

TEST(WorldTeam, TeamPlayerCountDecrementsOnRemove)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.addPlayer(1, "TeamA", 1, 0, Orientation::N);

    w.removePlayer(id);

    EXPECT_EQ(w.teamPlayerCount("TeamA"), 1);
}

TEST(WorldTeam, TeamPlayerCountIsolatedPerTeam)
{
    auto w = makeWorld(10, 10);
    w.addPlayer(0, "TeamA", 0, 0, Orientation::N);
    w.addPlayer(1, "TeamA", 1, 0, Orientation::N);
    w.addPlayer(2, "TeamB", 2, 0, Orientation::N);

    EXPECT_EQ(w.teamPlayerCount("TeamA"), 2);
    EXPECT_EQ(w.teamPlayerCount("TeamB"), 1);
}

TEST(WorldTeam, TeamPlayerCountUnknownTeamIsZero)
{
    auto w = makeWorld(10, 10);
    w.addPlayer(0, "TeamA", 0, 0, Orientation::N);

    EXPECT_EQ(w.teamPlayerCount("NoSuchTeam"), 0);
}