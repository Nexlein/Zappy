#include <gtest/gtest.h>

#include "core/GameState.hpp"

// Map initialization
TEST(GameStateTest, ApplyMapSize)
{
    GameState gs;
    MapSize event{10, 20};
    gs.applyEvent(event);

    EXPECT_EQ(gs.world.width, 10);
    EXPECT_EQ(gs.world.height, 20);
    EXPECT_EQ(gs.world.tiles.size(), 200);
}

TEST(GameStateTest, ApplyTileContent)
{
    GameState gs;
    gs.world.width = 5;
    gs.world.height = 5;
    gs.world.tiles.resize(25);

    TileContent event{2, 3, {10, 1, 2, 3, 4, 5, 6}};
    gs.applyEvent(event);

    auto& tile = gs.world.at(2, 3);
    EXPECT_EQ(tile.food, 10);
    EXPECT_EQ(tile.linemate, 1);
    EXPECT_EQ(tile.deraumere, 2);
    EXPECT_EQ(tile.sibur, 3);
    EXPECT_EQ(tile.mendiane, 4);
    EXPECT_EQ(tile.phiras, 5);
    EXPECT_EQ(tile.thystame, 6);
}

TEST(GameStateTest, ApplyTeamName)
{
    GameState gs;
    TeamName event1{"TeamA"};
    TeamName event2{"TeamB"};
    gs.applyEvent(event1);
    gs.applyEvent(event2);

    ASSERT_EQ(gs.world.teams.size(), 2);
    EXPECT_EQ(gs.world.teams[0], "TeamA");
    EXPECT_EQ(gs.world.teams[1], "TeamB");
}

// Player management
TEST(GameStateTest, ApplyPlayerNew)
{
    GameState gs;
    PlayerNew event{42, 5, 7, Orientation::E, 3, "TeamA"};
    gs.applyEvent(event);

    ASSERT_TRUE(gs.world.players.contains(42));
    auto& p = gs.world.players[42];
    EXPECT_EQ(p.id, 42);
    EXPECT_EQ(p.x, 5);
    EXPECT_EQ(p.y, 7);
    EXPECT_EQ(p.orientation, Orientation::E);
    EXPECT_EQ(p.level, 3);
    EXPECT_EQ(p.team, "TeamA");
    EXPECT_FALSE(p.incanting);
}

TEST(GameStateTest, ApplyPlayerPosition)
{
    GameState gs;
    gs.world.players[42] = Player{42, 0, 0, Orientation::N, 1, "TeamA"};

    PlayerPosition event{42, 10, 15, Orientation::S};
    gs.applyEvent(event);

    auto& p = gs.world.players[42];
    EXPECT_EQ(p.x, 10);
    EXPECT_EQ(p.y, 15);
    EXPECT_EQ(p.orientation, Orientation::S);
}

TEST(GameStateTest, ApplyPlayerPositionNonExistent)
{
    GameState gs;
    PlayerPosition event{999, 10, 15, Orientation::S};
    gs.applyEvent(event);  // Should not crash
    EXPECT_FALSE(gs.world.players.contains(999));
}

TEST(GameStateTest, ApplyPlayerLevel)
{
    GameState gs;
    gs.world.players[42] = Player{42, 0, 0, Orientation::N, 1, "TeamA"};

    PlayerLevel event{42, 5};
    gs.applyEvent(event);

    EXPECT_EQ(gs.world.players[42].level, 5);
}

TEST(GameStateTest, ApplyPlayerInventory)
{
    GameState gs;
    gs.world.players[42] = Player{42, 0, 0, Orientation::N, 1, "TeamA"};

    PlayerInventory event{42, 5, 7, {10, 1, 2, 3, 4, 5, 6}};
    gs.applyEvent(event);

    auto& inv = gs.world.players[42].inventory;
    EXPECT_EQ(inv.food, 10);
    EXPECT_EQ(inv.linemate, 1);
    EXPECT_EQ(inv.thystame, 6);
}

TEST(GameStateTest, ApplyPlayerDeath)
{
    GameState gs;
    gs.world.players[42] = Player{42, 0, 0, Orientation::N, 1, "TeamA"};
    gs.world.players[43] = Player{43, 1, 1, Orientation::E, 2, "TeamB"};

    PlayerDeath event{42};
    gs.applyEvent(event);

    EXPECT_FALSE(gs.world.players.contains(42));
    EXPECT_TRUE(gs.world.players.contains(43));
}

// Resource drop/take
TEST(GameStateTest, ApplyPlayerResourceDrop)
{
    GameState gs;
    gs.world.width = 10;
    gs.world.height = 10;
    gs.world.tiles.resize(100);

    gs.world.players[42] = Player{42, 5, 7, Orientation::N, 1, "TeamA"};
    gs.world.players[42].inventory.linemate = 5;
    gs.world.at(5, 7).linemate = 2;

    PlayerResourceDrop event{42, 1};  // resourceId 1 = linemate
    gs.applyEvent(event);

    EXPECT_EQ(gs.world.players[42].inventory.linemate, 4);
    EXPECT_EQ(gs.world.at(5, 7).linemate, 3);
}

TEST(GameStateTest, ApplyPlayerResourceTake)
{
    GameState gs;
    gs.world.width = 10;
    gs.world.height = 10;
    gs.world.tiles.resize(100);

    gs.world.players[42] = Player{42, 5, 7, Orientation::N, 1, "TeamA"};
    gs.world.players[42].inventory.sibur = 1;
    gs.world.at(5, 7).sibur = 3;

    PlayerResourceTake event{42, 3};  // resourceId 3 = sibur
    gs.applyEvent(event);

    EXPECT_EQ(gs.world.players[42].inventory.sibur, 2);
    EXPECT_EQ(gs.world.at(5, 7).sibur, 2);
}

// Incantation
TEST(GameStateTest, ApplyIncantationStart)
{
    GameState gs;
    gs.world.players[42] = Player{42, 5, 7, Orientation::N, 2, "TeamA"};
    gs.world.players[43] = Player{43, 5, 7, Orientation::E, 2, "TeamA"};
    gs.world.players[44] = Player{44, 1, 1, Orientation::S, 3, "TeamB"};

    IncantationStart event{5, 7, 2, {42, 43}};
    gs.applyEvent(event);

    EXPECT_TRUE(gs.world.players[42].incanting);
    EXPECT_TRUE(gs.world.players[43].incanting);
    EXPECT_FALSE(gs.world.players[44].incanting);
}

TEST(GameStateTest, ApplyIncantationStartNonExistentPlayer)
{
    GameState gs;
    gs.world.players[42] = Player{42, 5, 7, Orientation::N, 2, "TeamA"};

    IncantationStart event{5, 7, 2, {42, 999}};
    gs.applyEvent(event);  // Should not crash

    EXPECT_TRUE(gs.world.players[42].incanting);
}

TEST(GameStateTest, ApplyIncantationEnd)
{
    GameState gs;
    gs.world.players[42] = Player{42, 5, 7, Orientation::N, 2, "TeamA"};
    gs.world.players[43] = Player{43, 5, 7, Orientation::E, 2, "TeamA"};
    gs.world.players[44] = Player{44, 1, 1, Orientation::S, 3, "TeamB"};
    gs.world.players[42].incanting = true;
    gs.world.players[43].incanting = true;
    gs.world.players[44].incanting = true;

    IncantationEnd event{5, 7, true};
    gs.applyEvent(event);

    EXPECT_FALSE(gs.world.players[42].incanting);
    EXPECT_FALSE(gs.world.players[43].incanting);
    EXPECT_TRUE(gs.world.players[44].incanting);  // Different tile
}

// Eggs
TEST(GameStateTest, ApplyEggNew)
{
    GameState gs;
    gs.world.players[42] = Player{42, 5, 7, Orientation::N, 3, "TeamA"};

    EggNew event{1, 42, 10, 15};
    gs.applyEvent(event);

    ASSERT_TRUE(gs.world.eggs.contains(1));
    auto& egg = gs.world.eggs[1];
    EXPECT_EQ(egg.id, 1);
    EXPECT_EQ(egg.x, 10);
    EXPECT_EQ(egg.y, 15);
    EXPECT_EQ(egg.team, "TeamA");
}

TEST(GameStateTest, ApplyEggNewNonExistentPlayer)
{
    GameState gs;
    EggNew event{1, 999, 10, 15};
    gs.applyEvent(event);  // Should not crash

    EXPECT_FALSE(gs.world.eggs.contains(1));
}

TEST(GameStateTest, ApplyEggHatch)
{
    GameState gs;
    gs.world.eggs[1] = Egg{1, 5, 7, "TeamA"};
    gs.world.eggs[2] = Egg{2, 10, 15, "TeamB"};

    EggHatch event{1};
    gs.applyEvent(event);

    EXPECT_FALSE(gs.world.eggs.contains(1));
    EXPECT_TRUE(gs.world.eggs.contains(2));
}

TEST(GameStateTest, ApplyEggDeath)
{
    GameState gs;
    gs.world.eggs[1] = Egg{1, 5, 7, "TeamA"};
    gs.world.eggs[2] = Egg{2, 10, 15, "TeamB"};

    EggDeath event{1};
    gs.applyEvent(event);

    EXPECT_FALSE(gs.world.eggs.contains(1));
    EXPECT_TRUE(gs.world.eggs.contains(2));
}

// Time unit
TEST(GameStateTest, ApplyTimeUnit)
{
    GameState gs;
    gs.timeUnit = 100;

    TimeUnit event{250};
    gs.applyEvent(event);

    EXPECT_EQ(gs.timeUnit, 250);
}

TEST(GameStateTest, ApplyTimeUnitChange)
{
    GameState gs;
    gs.timeUnit = 100;

    TimeUnitChange event{300};
    gs.applyEvent(event);

    EXPECT_EQ(gs.timeUnit, 300);
}

// Game end
TEST(GameStateTest, ApplyGameEnd)
{
    GameState gs;
    GameEnd event{"TeamA"};
    gs.applyEvent(event);

    EXPECT_EQ(gs.winnerTeam, "TeamA");
}

TEST(GameStateTest, ApplyGameEndWithSpaces)
{
    GameState gs;
    GameEnd event{"team swag"};
    gs.applyEvent(event);

    EXPECT_EQ(gs.winnerTeam, "team swag");
}

// Multiple events sequencing
TEST(GameStateTest, MultipleEventsSequence)
{
    GameState gs;

    gs.applyEvent(MapSize{10, 10});
    gs.applyEvent(TeamName{"TeamA"});
    gs.applyEvent(PlayerNew{42, 5, 7, Orientation::E, 1, "TeamA"});
    gs.applyEvent(PlayerLevel{42, 2});
    gs.applyEvent(PlayerPosition{42, 6, 8, Orientation::N});

    EXPECT_EQ(gs.world.width, 10);
    EXPECT_EQ(gs.world.height, 10);
    ASSERT_EQ(gs.world.teams.size(), 1);
    EXPECT_EQ(gs.world.teams[0], "TeamA");
    ASSERT_TRUE(gs.world.players.contains(42));
    EXPECT_EQ(gs.world.players[42].level, 2);
    EXPECT_EQ(gs.world.players[42].x, 6);
    EXPECT_EQ(gs.world.players[42].y, 8);
    EXPECT_EQ(gs.world.players[42].orientation, Orientation::N);
}

// Edge case: resource operations on non-existent player
TEST(GameStateTest, ResourceOperationsNonExistentPlayer)
{
    GameState gs;
    gs.world.width = 10;
    gs.world.height = 10;
    gs.world.tiles.resize(100);

    PlayerResourceDrop dropEvent{999, 1};
    gs.applyEvent(dropEvent);  // Should not crash

    PlayerResourceTake takeEvent{999, 2};
    gs.applyEvent(takeEvent);  // Should not crash
}
