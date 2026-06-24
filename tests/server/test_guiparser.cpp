#include <gtest/gtest.h>

#include <variant>

#include "protocol/GuiParser.hpp"

TEST(GuiParser, Msz)
{
    auto result = GuiParser::parse("msz");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Gui::Msz>(*result));
}

TEST(GuiParser, Mct)
{
    auto result = GuiParser::parse("mct");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Gui::Mct>(*result));
}

TEST(GuiParser, Tna)
{
    auto result = GuiParser::parse("tna");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Gui::Tna>(*result));
}

TEST(GuiParser, Sgt)
{
    auto result = GuiParser::parse("sgt");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Gui::Sgt>(*result));
}

TEST(GuiParser, BctValid)
{
    auto result = GuiParser::parse("bct 3 5");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Gui::Bct>(*result));
    auto bct = std::get<Gui::Bct>(*result);
    EXPECT_EQ(bct.x, 3);
    EXPECT_EQ(bct.y, 5);
}

TEST(GuiParser, BctZero)
{
    auto result = GuiParser::parse("bct 0 0");
    ASSERT_TRUE(result.has_value());
    auto bct = std::get<Gui::Bct>(*result);
    EXPECT_EQ(bct.x, 0);
    EXPECT_EQ(bct.y, 0);
}

TEST(GuiParser, PpoValid)
{
    auto result = GuiParser::parse("ppo #7");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Gui::Ppo>(*result));
    EXPECT_EQ(std::get<Gui::Ppo>(*result).id, 7);
}

TEST(GuiParser, PlvValid)
{
    auto result = GuiParser::parse("plv #2");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Gui::Plv>(*result));
    EXPECT_EQ(std::get<Gui::Plv>(*result).id, 2);
}

TEST(GuiParser, PinValid)
{
    auto result = GuiParser::parse("pin #0");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Gui::Pin>(*result));
    EXPECT_EQ(std::get<Gui::Pin>(*result).id, 0);
}

TEST(GuiParser, SstValid)
{
    auto result = GuiParser::parse("sst 42");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Gui::Sst>(*result));
    EXPECT_EQ(std::get<Gui::Sst>(*result).freq, 42);
}

TEST(GuiParser, GttValid)
{
    auto result = GuiParser::parse("gtt TeamA");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Gui::Gtt>(*result));
    EXPECT_EQ(std::get<Gui::Gtt>(*result).team, "TeamA");
}

TEST(GuiParser, GttMissingTeam) { EXPECT_FALSE(GuiParser::parse("gtt ").has_value()); }
TEST(GuiParser, GttExtraArg) { EXPECT_FALSE(GuiParser::parse("gtt TeamA B").has_value()); }

TEST(GuiParser, PpoMissingHash) { EXPECT_FALSE(GuiParser::parse("ppo 7").has_value()); }
TEST(GuiParser, BctMissingY) { EXPECT_FALSE(GuiParser::parse("bct 3").has_value()); }
TEST(GuiParser, BctBadArgs) { EXPECT_FALSE(GuiParser::parse("bct abc def").has_value()); }
TEST(GuiParser, SstBadArg) { EXPECT_FALSE(GuiParser::parse("sst abc").has_value()); }
TEST(GuiParser, UnknownCommand) { EXPECT_FALSE(GuiParser::parse("garbage").has_value()); }
TEST(GuiParser, EmptyLine) { EXPECT_FALSE(GuiParser::parse("").has_value()); }
TEST(GuiParser, CaseSensitive) { EXPECT_FALSE(GuiParser::parse("MSZ").has_value()); }