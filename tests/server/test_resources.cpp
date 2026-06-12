#include <gtest/gtest.h>

#include "../../server/src/core/data/Resources.hpp"

TEST(ResourcesTest, DefaultZero)
{
    Resources r;
    EXPECT_TRUE(r.isEmpty());
}

TEST(ResourcesTest, IsEmptyFalseWhenAnySet)
{
    Resources r;
    r.food = 1;
    EXPECT_FALSE(r.isEmpty());
}

TEST(ResourcesTest, IndexOperatorRead)
{
    Resources r;
    r.food = 1;
    r.linemate = 2;
    r.thystame = 7;

    EXPECT_EQ(r[ResourceType::FOOD], 1);
    EXPECT_EQ(r[ResourceType::LINEMATE], 2);
    EXPECT_EQ(r[ResourceType::THYSTAME], 7);
}

TEST(ResourcesTest, IndexOperatorWrite)
{
    Resources r;
    r[ResourceType::SIBUR] = 42;
    EXPECT_EQ(r.sibur, 42);
}

TEST(ResourcesTest, AllTypesIndexable)
{
    Resources r;
    for (int i = 0; i < Resources::TYPE_COUNT; i++) {
        auto type = static_cast<ResourceType>(i);
        EXPECT_NO_THROW(r[type] = i);
        EXPECT_EQ(r[type], i);
    }
}

TEST(ResourcesTest, DensityAllPositive)
{
    for (int i = 0; i < Resources::TYPE_COUNT; i++) {
        auto type = static_cast<ResourceType>(i);
        EXPECT_GT(Resources::density(type), 0.0f);
    }
}

TEST(ResourcesTest, DensityFoodHighest)
{
    EXPECT_GT(Resources::density(ResourceType::FOOD), Resources::density(ResourceType::LINEMATE));
    EXPECT_GT(Resources::density(ResourceType::LINEMATE),
              Resources::density(ResourceType::THYSTAME));
}

TEST(ResourcesTest, DensityThystameLowest)
{
    for (int i = 0; i < Resources::TYPE_COUNT - 1; i++) {
        auto type = static_cast<ResourceType>(i);
        if (type == ResourceType::THYSTAME) continue;
        EXPECT_GE(Resources::density(type), Resources::density(ResourceType::THYSTAME));
    }
}

TEST(ResourcesTest, TypeCountCorrect) { EXPECT_EQ(Resources::TYPE_COUNT, 7); }
