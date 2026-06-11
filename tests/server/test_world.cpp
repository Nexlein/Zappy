#include <gtest/gtest.h>

#include "../../server/src/core/World.hpp"

static World makeWorld(int w, int h)
{
    return World(w, h, {"TeamA", "TeamB"}, 5);
}

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
            for (int y = 0; y < 10; y++)
                total += w.at(x, y).resources[type];

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
            for (int y = 0; y < 2; y++)
                total += w.at(x, y).resources[type];

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
            before += w.at(x, y).resources[static_cast<ResourceType>(1)]; // linemate

    w.spawnResources();

    int after = 0;
    for (int x = 0; x < 10; x++)
        for (int y = 0; y < 10; y++)
            after += w.at(x, y).resources[static_cast<ResourceType>(1)];

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
    EXPECT_EQ(w.getPlayer(id).inventory[ResourceType::FOOD], 0);
    EXPECT_EQ(w.at(2, 2).resources[ResourceType::FOOD], 1);
}

TEST(WorldInventory, SetResourceFailsWhenInventoryEmpty)
{
    auto w = makeWorld(10, 10);
    int id = w.addPlayer(0, "TeamA", 0, 0, Orientation::N);

    bool ok = w.setResource(id, ResourceType::SIBUR);

    EXPECT_FALSE(ok);
}
