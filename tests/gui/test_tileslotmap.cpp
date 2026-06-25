#include <gtest/gtest.h>

#include "core/Resources.hpp"
#include "renderer/raylib_helpers/TileSlotMap.hpp"

// ---- slotOffset ----

TEST(TileSlotMap, SlotOffsetAllIndices)
{
    // All 8 offsets must be non-zero and within ±0.36 in both axes
    for (int i = 0; i < 8; i++) {
        auto [dx, dz] = TileSlotMap::slotOffset(i);
        // At least one axis must be non-zero — center (0,0) is reserved for players
        EXPECT_TRUE(dx != 0.0f || dz != 0.0f) << "slot " << i << " is at tile center";
        EXPECT_LE(std::abs(dx), 0.36f) << "slot " << i;
        EXPECT_LE(std::abs(dz), 0.36f) << "slot " << i;
    }
}

TEST(TileSlotMap, SlotOffsetDistinct)
{
    // All 8 slots must have distinct (dx, dz) pairs
    std::vector<std::pair<float, float>> offsets;
    for (int i = 0; i < 8; i++) offsets.push_back(TileSlotMap::slotOffset(i));
    for (int i = 0; i < 8; i++)
        for (int j = i + 1; j < 8; j++)
            EXPECT_NE(offsets[i], offsets[j]) << "slots " << i << " and " << j << " collide";
}

// ---- updateResourceSlots ----

TEST(TileSlotMap, ResourceSlotAssignedOnAppearance)
{
    TileSlotMap map;
    Resources res;
    res[0] = 1;  // food appears

    auto slots = map.updateResourceSlots(0, 0, res);
    EXPECT_GE(slots[0], 0);
    for (int i = 1; i < 7; i++) EXPECT_EQ(slots[i], -1);
}

TEST(TileSlotMap, ResourceSlotStableAcrossFrames)
{
    TileSlotMap map;
    Resources res;
    res[2] = 3;

    auto slots1 = map.updateResourceSlots(1, 1, res);
    auto slots2 = map.updateResourceSlots(1, 1, res);
    EXPECT_EQ(slots1[2], slots2[2]);
}

TEST(TileSlotMap, ResourceSlotReleasedOnDisappearance)
{
    TileSlotMap map;
    Resources res;
    res[1] = 1;
    map.updateResourceSlots(0, 0, res);

    res[1] = 0;
    auto slots = map.updateResourceSlots(0, 0, res);
    EXPECT_EQ(slots[1], -1);
}

TEST(TileSlotMap, ResourceSlotsDistinctPerType)
{
    TileSlotMap map;
    Resources res;
    for (int i = 0; i < 7; i++) res[i] = 1;

    auto slots = map.updateResourceSlots(0, 0, res);
    for (int i = 0; i < 7; i++)
        for (int j = i + 1; j < 7; j++)
            EXPECT_NE(slots[i], slots[j])
                << "resource types " << i << " and " << j << " share slot";
}

// ---- syncEggs / eggSlot ----

TEST(TileSlotMap, EggSlotAssignedOnAppearance)
{
    TileSlotMap map;
    std::unordered_map<int, Egg> eggs;
    eggs[42] = Egg{.id = 42, .x = 0, .y = 0, .team = "A"};

    map.syncEggs(eggs);
    EXPECT_GE(map.eggSlot(42), 0);
}

TEST(TileSlotMap, EggSlotStableAcrossFrames)
{
    TileSlotMap map;
    std::unordered_map<int, Egg> eggs;
    eggs[7] = Egg{.id = 7, .x = 2, .y = 3, .team = "B"};

    map.syncEggs(eggs);
    int slot1 = map.eggSlot(7);
    map.syncEggs(eggs);
    int slot2 = map.eggSlot(7);
    EXPECT_EQ(slot1, slot2);
}

TEST(TileSlotMap, EggSlotReleasedOnDisappearance)
{
    TileSlotMap map;
    std::unordered_map<int, Egg> eggs;
    eggs[5] = Egg{.id = 5, .x = 0, .y = 0, .team = "C"};

    map.syncEggs(eggs);
    eggs.clear();
    map.syncEggs(eggs);
    EXPECT_EQ(map.eggSlot(5), -1);
}

TEST(TileSlotMap, EggAndResourceDontShareSlot)
{
    TileSlotMap map;
    Resources res;
    for (int i = 0; i < 7; i++) res[i] = 1;
    auto slots = map.updateResourceSlots(0, 0, res);

    std::unordered_map<int, Egg> eggs;
    eggs[1] = Egg{.id = 1, .x = 0, .y = 0, .team = "A"};
    map.syncEggs(eggs);

    int eSlot = map.eggSlot(1);
    // egg slot must not collide with any of the 7 resource slots
    for (int i = 0; i < 7; i++)
        EXPECT_NE(eSlot, slots[i]) << "egg collides with resource type " << i;
}

// ---- clear ----

TEST(TileSlotMap, ClearResetsState)
{
    TileSlotMap map;
    Resources res;
    res[0] = 1;
    map.updateResourceSlots(0, 0, res);

    std::unordered_map<int, Egg> eggs;
    eggs[99] = Egg{.id = 99, .x = 0, .y = 0, .team = "X"};
    map.syncEggs(eggs);

    map.clear();

    EXPECT_EQ(map.eggSlot(99), -1);
    auto slots = map.updateResourceSlots(0, 0, res);
    // after clear, resource gets a fresh slot (not -1, since it's present)
    EXPECT_GE(slots[0], 0);
}
