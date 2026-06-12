#include <gtest/gtest.h>

#include <cassert>
#include <variant>

#include "core/data/Resources.hpp"
#include "protocol/AiParser.hpp"

TEST(AiParser, Forward)
{
    auto result = AiParser::parse("Forward");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Forward>(*result));
}

TEST(AiParser, TakeLinemate)
{
    auto result = AiParser::parse("Take linemate");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Ai::Take>(*result));
    EXPECT_EQ(std::get<Ai::Take>(*result).resource, ResourceType::LINEMATE);
}

TEST(AiParser, UnknownCommand) { EXPECT_FALSE(AiParser::parse("garbage").has_value()); }

TEST(AiParser, Right)
{
    auto result = AiParser::parse("Right");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Right>(*result));
}

TEST(AiParser, Left)
{
    auto result = AiParser::parse("Left");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Left>(*result));
}

TEST(AiParser, Look)
{
    auto result = AiParser::parse("Look");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Look>(*result));
}

TEST(AiParser, Inventory)
{
    auto result = AiParser::parse("Inventory");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Inventory>(*result));
}

TEST(AiParser, Fork)
{
    auto result = AiParser::parse("Fork");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Fork>(*result));
}

TEST(AiParser, Eject)
{
    auto result = AiParser::parse("Eject");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Eject>(*result));
}

TEST(AiParser, Incantation)
{
    auto result = AiParser::parse("Incantation");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::Incantation>(*result));
}

TEST(AiParser, ConnectNbr)
{
    auto result = AiParser::parse("Connect_nbr");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::holds_alternative<Ai::ConnectNbr>(*result));
}

TEST(AiParser, BroadcastWithSpaces)
{
    auto result = AiParser::parse("Broadcast hello world");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Ai::Broadcast>(*result));
    EXPECT_EQ(std::get<Ai::Broadcast>(*result).message, "hello world");
}

TEST(AiParser, SetFood)
{
    auto result = AiParser::parse("Set food");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<Ai::Set>(*result));
    EXPECT_EQ(std::get<Ai::Set>(*result).resource, ResourceType::FOOD);
}

TEST(AiParser, TakeAllResources)
{
    using RT = ResourceType;
    const std::pair<std::string, RT> cases[] = {
        {"Take food", RT::FOOD},           {"Take linemate", RT::LINEMATE},
        {"Take deraumere", RT::DERAUMERE}, {"Take sibur", RT::SIBUR},
        {"Take mendiane", RT::MENDIANE},   {"Take phiras", RT::PHIRAS},
        {"Take thystame", RT::THYSTAME},
    };
    for (auto& [input, expected] : cases) {
        auto result = AiParser::parse(input);
        ASSERT_TRUE(result.has_value()) << input;
        ASSERT_TRUE(std::holds_alternative<Ai::Take>(*result)) << input;
        EXPECT_EQ(std::get<Ai::Take>(*result).resource, expected) << input;
    }
}

TEST(AiParser, TakeBadResource) { EXPECT_FALSE(AiParser::parse("Take badresource").has_value()); }
TEST(AiParser, TakeNoArg) { EXPECT_FALSE(AiParser::parse("Take").has_value()); }
TEST(AiParser, SetNoArg) { EXPECT_FALSE(AiParser::parse("Set").has_value()); }
TEST(AiParser, EmptyLine) { EXPECT_FALSE(AiParser::parse("").has_value()); }
TEST(AiParser, CaseSensitive) { EXPECT_FALSE(AiParser::parse("forward").has_value()); }