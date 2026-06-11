#include <gtest/gtest.h>

#include "core/World.hpp"

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
