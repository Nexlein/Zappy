#include <gtest/gtest.h>
#include "network/ProtocolParser.hpp"

// Map commands
TEST(ProtocolParserTest, ParseMSZ) {
    auto event = ProtocolParser::parse("msz 10 20\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<MapSize>(*event));

    auto ms = std::get<MapSize>(*event);
    EXPECT_EQ(ms.width, 10);
    EXPECT_EQ(ms.height, 20);
}

TEST(ProtocolParserTest, ParseBCT) {
    auto event = ProtocolParser::parse("bct 5 7 10 1 2 3 4 5 6\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<TileContent>(*event));

    auto tc = std::get<TileContent>(*event);
    EXPECT_EQ(tc.x, 5);
    EXPECT_EQ(tc.y, 7);
    EXPECT_EQ(tc.resources.food, 10);
    EXPECT_EQ(tc.resources.linemate, 1);
    EXPECT_EQ(tc.resources.deraumere, 2);
    EXPECT_EQ(tc.resources.sibur, 3);
    EXPECT_EQ(tc.resources.mendiane, 4);
    EXPECT_EQ(tc.resources.phiras, 5);
    EXPECT_EQ(tc.resources.thystame, 6);
}

TEST(ProtocolParserTest, ParseTNA) {
    auto event = ProtocolParser::parse("tna TeamA\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<TeamName>(*event));
    EXPECT_EQ(std::get<TeamName>(*event).name, "TeamA");
}

TEST(ProtocolParserTest, ParseTNAWithSpaces) {
    auto event = ProtocolParser::parse("tna team swag\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<TeamName>(*event));
    EXPECT_EQ(std::get<TeamName>(*event).name, "team swag");
}

// Player commands
TEST(ProtocolParserTest, ParsePNW) {
    auto event = ProtocolParser::parse("pnw #42 5 7 2 3 TeamA\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerNew>(*event));

    auto pn = std::get<PlayerNew>(*event);
    EXPECT_EQ(pn.id, 42);
    EXPECT_EQ(pn.x, 5);
    EXPECT_EQ(pn.y, 7);
    EXPECT_EQ(pn.orientation, Orientation::E);
    EXPECT_EQ(pn.level, 3);
    EXPECT_EQ(pn.team, "TeamA");
}

TEST(ProtocolParserTest, ParsePPO) {
    auto event = ProtocolParser::parse("ppo #42 5 7 1\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerPosition>(*event));

    auto pp = std::get<PlayerPosition>(*event);
    EXPECT_EQ(pp.id, 42);
    EXPECT_EQ(pp.x, 5);
    EXPECT_EQ(pp.y, 7);
    EXPECT_EQ(pp.orientation, Orientation::N);
}

TEST(ProtocolParserTest, ParsePLV) {
    auto event = ProtocolParser::parse("plv #42 5\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerLevel>(*event));

    auto pl = std::get<PlayerLevel>(*event);
    EXPECT_EQ(pl.id, 42);
    EXPECT_EQ(pl.level, 5);
}

TEST(ProtocolParserTest, ParsePIN) {
    auto event = ProtocolParser::parse("pin #42 5 7 10 1 2 3 4 5 6\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerInventory>(*event));

    auto pi = std::get<PlayerInventory>(*event);
    EXPECT_EQ(pi.id, 42);
    EXPECT_EQ(pi.x, 5);
    EXPECT_EQ(pi.y, 7);
    EXPECT_EQ(pi.inventory.food, 10);
    EXPECT_EQ(pi.inventory.thystame, 6);
}

TEST(ProtocolParserTest, ParsePEX) {
    auto event = ProtocolParser::parse("pex #42\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerExpulsion>(*event));
    EXPECT_EQ(std::get<PlayerExpulsion>(*event).id, 42);
}

TEST(ProtocolParserTest, ParsePBC) {
    auto event = ProtocolParser::parse("pbc #42 hello\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerBroadcast>(*event));

    auto pb = std::get<PlayerBroadcast>(*event);
    EXPECT_EQ(pb.id, 42);
    EXPECT_EQ(pb.message, "hello");
}

TEST(ProtocolParserTest, ParsePBCWithSpaces) {
    auto event = ProtocolParser::parse("pbc #42 hello world test\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerBroadcast>(*event));

    auto pb = std::get<PlayerBroadcast>(*event);
    EXPECT_EQ(pb.id, 42);
    EXPECT_EQ(pb.message, "hello world test");
}

TEST(ProtocolParserTest, ParsePIC) {
    auto event = ProtocolParser::parse("pic 5 7 3 #42 #43 #44\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<IncantationStart>(*event));

    auto is = std::get<IncantationStart>(*event);
    EXPECT_EQ(is.x, 5);
    EXPECT_EQ(is.y, 7);
    EXPECT_EQ(is.level, 3);
    ASSERT_EQ(is.playerIds.size(), 3);
    EXPECT_EQ(is.playerIds[0], 42);
    EXPECT_EQ(is.playerIds[1], 43);
    EXPECT_EQ(is.playerIds[2], 44);
}

TEST(ProtocolParserTest, ParsePIESuccess) {
    auto event = ProtocolParser::parse("pie 5 7 1\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<IncantationEnd>(*event));

    auto ie = std::get<IncantationEnd>(*event);
    EXPECT_EQ(ie.x, 5);
    EXPECT_EQ(ie.y, 7);
    EXPECT_TRUE(ie.success);
}

TEST(ProtocolParserTest, ParsePIEFailure) {
    auto event = ProtocolParser::parse("pie 5 7 0\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<IncantationEnd>(*event));

    auto ie = std::get<IncantationEnd>(*event);
    EXPECT_EQ(ie.x, 5);
    EXPECT_EQ(ie.y, 7);
    EXPECT_FALSE(ie.success);
}

TEST(ProtocolParserTest, ParsePFK) {
    auto event = ProtocolParser::parse("pfk #42\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerFork>(*event));
    EXPECT_EQ(std::get<PlayerFork>(*event).id, 42);
}

TEST(ProtocolParserTest, ParsePDR) {
    auto event = ProtocolParser::parse("pdr #42 3\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerResourceDrop>(*event));

    auto prd = std::get<PlayerResourceDrop>(*event);
    EXPECT_EQ(prd.playerId, 42);
    EXPECT_EQ(prd.resourceId, 3);
}

TEST(ProtocolParserTest, ParsePGT) {
    auto event = ProtocolParser::parse("pgt #42 5\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerResourceTake>(*event));

    auto prt = std::get<PlayerResourceTake>(*event);
    EXPECT_EQ(prt.playerId, 42);
    EXPECT_EQ(prt.resourceId, 5);
}

TEST(ProtocolParserTest, ParsePDI) {
    auto event = ProtocolParser::parse("pdi #42\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<PlayerDeath>(*event));
    EXPECT_EQ(std::get<PlayerDeath>(*event).id, 42);
}

// Egg commands
TEST(ProtocolParserTest, ParseENW) {
    auto event = ProtocolParser::parse("enw #1 #42 5 7\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<EggNew>(*event));

    auto en = std::get<EggNew>(*event);
    EXPECT_EQ(en.eggId, 1);
    EXPECT_EQ(en.playerId, 42);
    EXPECT_EQ(en.x, 5);
    EXPECT_EQ(en.y, 7);
}

TEST(ProtocolParserTest, ParseEBO) {
    auto event = ProtocolParser::parse("ebo #1\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<EggHatch>(*event));
    EXPECT_EQ(std::get<EggHatch>(*event).id, 1);
}

TEST(ProtocolParserTest, ParseEDI) {
    auto event = ProtocolParser::parse("edi #1\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<EggDeath>(*event));
    EXPECT_EQ(std::get<EggDeath>(*event).id, 1);
}

// Server/Meta commands
TEST(ProtocolParserTest, ParseSGT) {
    auto event = ProtocolParser::parse("sgt 100\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<TimeUnit>(*event));
    EXPECT_EQ(std::get<TimeUnit>(*event).timeUnit, 100);
}

TEST(ProtocolParserTest, ParseSST) {
    auto event = ProtocolParser::parse("sst 200\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<TimeUnitChange>(*event));
    EXPECT_EQ(std::get<TimeUnitChange>(*event).timeUnit, 200);
}

TEST(ProtocolParserTest, ParseSEG) {
    auto event = ProtocolParser::parse("seg TeamA\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<GameEnd>(*event));
    EXPECT_EQ(std::get<GameEnd>(*event).winningTeam, "TeamA");
}

TEST(ProtocolParserTest, ParseSEGWithSpaces) {
    auto event = ProtocolParser::parse("seg team swag\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<GameEnd>(*event));
    EXPECT_EQ(std::get<GameEnd>(*event).winningTeam, "team swag");
}

TEST(ProtocolParserTest, ParseSMG) {
    auto event = ProtocolParser::parse("smg test message\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<ServerMessage>(*event));
    EXPECT_EQ(std::get<ServerMessage>(*event).message, "test message");
}

TEST(ProtocolParserTest, ParseSUC) {
    auto event = ProtocolParser::parse("suc\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<UnknownCommand>(*event));
}

TEST(ProtocolParserTest, ParseSBP) {
    auto event = ProtocolParser::parse("sbp\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<BadParameters>(*event));
}

// Error cases
TEST(ProtocolParserTest, ParseEmptyString) {
    auto event = ProtocolParser::parse("");
    EXPECT_FALSE(event.has_value());
}

TEST(ProtocolParserTest, ParseUnknownCommand) {
    auto event = ProtocolParser::parse("xyz 1 2 3\n");
    EXPECT_FALSE(event.has_value());
}

TEST(ProtocolParserTest, ParseMalformedMSZ) {
    auto event = ProtocolParser::parse("msz 10\n");  // Missing height
    EXPECT_FALSE(event.has_value());
}

TEST(ProtocolParserTest, ParseMalformedBCT) {
    auto event = ProtocolParser::parse("bct 5 7 1 2 3\n");  // Missing resources
    EXPECT_FALSE(event.has_value());
}

TEST(ProtocolParserTest, ParseInvalidNumber) {
    auto event = ProtocolParser::parse("msz abc def\n");
    EXPECT_FALSE(event.has_value());
}

TEST(ProtocolParserTest, ParseNoNewline) {
    auto event = ProtocolParser::parse("msz 10 20");
    ASSERT_TRUE(event.has_value());  // Should still work (stopAt defaults to \n, optional)
    ASSERT_TRUE(std::holds_alternative<MapSize>(*event));
}

TEST(ProtocolParserTest, ParseMultipleLines) {
    // Parser should only process first line
    auto event = ProtocolParser::parse("msz 10 20\nbct 0 0 1 2 3 4 5 6 7\n");
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<MapSize>(*event));

    auto ms = std::get<MapSize>(*event);
    EXPECT_EQ(ms.width, 10);
    EXPECT_EQ(ms.height, 20);
}

TEST(ProtocolParserTest, ParseOrientations) {
    // Test all 4 orientations
    auto n = ProtocolParser::parse("ppo #1 0 0 1\n");
    EXPECT_EQ(std::get<PlayerPosition>(*n).orientation, Orientation::N);

    auto e = ProtocolParser::parse("ppo #1 0 0 2\n");
    EXPECT_EQ(std::get<PlayerPosition>(*e).orientation, Orientation::E);

    auto s = ProtocolParser::parse("ppo #1 0 0 3\n");
    EXPECT_EQ(std::get<PlayerPosition>(*s).orientation, Orientation::S);

    auto w = ProtocolParser::parse("ppo #1 0 0 4\n");
    EXPECT_EQ(std::get<PlayerPosition>(*w).orientation, Orientation::W);
}
