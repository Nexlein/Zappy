#include <gtest/gtest.h>

#include "network/Connection.hpp"

TEST(Connection, NextLineEmptyBuffer)
{
    Connection c(-1);
    EXPECT_FALSE(c.nextLine().has_value());
}

TEST(Connection, NextLinePartialLine)
{
    Connection c(-1);
    c.appendRead("Forward");
    EXPECT_FALSE(c.nextLine().has_value());
}

TEST(Connection, NextLineCompleteLine)
{
    Connection c(-1);
    c.appendRead("Forward\n");
    auto line = c.nextLine();
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(*line, "Forward");
}

TEST(Connection, NextLineConsumesOnlyFirstLine)
{
    Connection c(-1);
    c.appendRead("Forward\nRight\n");
    EXPECT_EQ(*c.nextLine(), "Forward");
    EXPECT_EQ(*c.nextLine(), "Right");
    EXPECT_FALSE(c.nextLine().has_value());
}

TEST(Connection, NextLineChunkedInput)
{
    Connection c(-1);
    c.appendRead("For");
    EXPECT_FALSE(c.nextLine().has_value());
    c.appendRead("ward\n");
    auto line = c.nextLine();
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(*line, "Forward");
}

TEST(Connection, NextLineEmptyLineAllowed)
{
    Connection c(-1);
    c.appendRead("\n");
    auto line = c.nextLine();
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(*line, "");
}

TEST(Connection, QueueWriteSetsPending)
{
    Connection c(-1);
    EXPECT_FALSE(c.hasPendingWrite());
    c.queueWrite("hello\n");
    EXPECT_TRUE(c.hasPendingWrite());
}

TEST(Connection, DefaultType)
{
    Connection c(-1);
    EXPECT_EQ(c.type(), ClientType::PENDING);
}

TEST(Connection, SetType)
{
    Connection c(-1);
    c.setType(ClientType::AI);
    EXPECT_EQ(c.type(), ClientType::AI);
    c.setType(ClientType::GUI);
    EXPECT_EQ(c.type(), ClientType::GUI);
}

TEST(Connection, DefaultPlayerId)
{
    Connection c(-1);
    EXPECT_EQ(c.playerId(), -1);
}

TEST(Connection, SetPlayerId)
{
    Connection c(-1);
    c.setPlayerId(42);
    EXPECT_EQ(c.playerId(), 42);
}
