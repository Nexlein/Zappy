#include <gtest/gtest.h>

#include "../../server/src/core/Args.hpp"

// Valid full args
TEST(ServerArgsTest, ValidAllFlags)
{
    const char* argv[] = {"prog", "-p",    "4242",  "-x", "10", "-y", "10",
                          "-n",   "TeamA", "TeamB", "-c", "5",  "-f", "100"};
    Args args(14, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_FALSE(args.isHelpRequested());
    EXPECT_EQ(args.exitCode(), 0);

    auto cfg = args.getConfig();
    EXPECT_EQ(cfg.port, 4242);
    EXPECT_EQ(cfg.width, 10);
    EXPECT_EQ(cfg.height, 10);
    EXPECT_EQ(cfg.clientsNb, 5);
    EXPECT_EQ(cfg.freq, 100);
    ASSERT_EQ(cfg.teamNames.size(), 2u);
    EXPECT_EQ(cfg.teamNames[0], "TeamA");
    EXPECT_EQ(cfg.teamNames[1], "TeamB");
}

TEST(ServerArgsTest, SingleTeam)
{
    const char* argv[] = {"prog", "-p",   "4242", "-x", "5",  "-y", "5",
                          "-n",   "Solo", "-c",   "1",  "-f", "50"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().teamNames.size(), 1u);
    EXPECT_EQ(args.getConfig().teamNames[0], "Solo");
}

// Help
TEST(ServerArgsTest, HelpFlag)
{
    const char* argv[] = {"prog", "--help"};
    Args args(2, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_TRUE(args.isHelpRequested());
    EXPECT_EQ(args.exitCode(), 0);
}

// Missing required flags
TEST(ServerArgsTest, NoArguments)
{
    const char* argv[] = {"prog"};
    Args args(1, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, MissingPort)
{
    const char* argv[] = {"prog", "-x", "10", "-y", "10", "-n", "TeamA", "-c", "5", "-f", "100"};
    Args args(11, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, MissingWidth)
{
    const char* argv[] = {"prog", "-p", "4242", "-y", "10", "-n", "TeamA", "-c", "5", "-f", "100"};
    Args args(11, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, MissingHeight)
{
    const char* argv[] = {"prog", "-p", "4242", "-x", "10", "-n", "TeamA", "-c", "5", "-f", "100"};
    Args args(11, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, MissingTeams)
{
    const char* argv[] = {"prog", "-p", "4242", "-x", "10", "-y", "10", "-c", "5", "-f", "100"};
    Args args(11, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, MissingClientsUsesDefault)
{
    const char* argv[] = {"prog", "-p", "4242", "-x", "10", "-y", "10", "-n", "TeamA", "-f", "100"};
    Args args(11, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().clientsNb, 10);
}

// Invalid values
TEST(ServerArgsTest, InvalidPort)
{
    const char* argv[] = {"prog", "-p",    "notaport", "-x", "10", "-y", "10",
                          "-n",   "TeamA", "-c",       "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, NegativeWidth)
{
    const char* argv[] = {"prog", "-p",    "4242", "-x", "-1", "-y", "10",
                          "-n",   "TeamA", "-c",   "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, ZeroHeight)
{
    const char* argv[] = {"prog", "-p",    "4242", "-x", "10", "-y", "0",
                          "-n",   "TeamA", "-c",   "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

// Reserved team name
TEST(ServerArgsTest, GraphicTeamNameRejected)
{
    const char* argv[] = {"prog", "-p",      "4242", "-x", "10", "-y", "10",
                          "-n",   "GRAPHIC", "-c",   "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ServerArgsTest, GraphicMixedWithValidTeams)
{
    const char* argv[] = {"prog", "-p",    "4242",    "-x", "10", "-y", "10",
                          "-n",   "TeamA", "GRAPHIC", "-c", "5",  "-f", "100"};
    Args args(14, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

// Port boundaries
TEST(ServerArgsTest, PortBoundaryLow)
{
    const char* argv[] = {"prog", "-p",    "1",  "-x", "10", "-y", "10",
                          "-n",   "TeamA", "-c", "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().port, 1);
}

TEST(ServerArgsTest, PortBoundaryHigh)
{
    const char* argv[] = {"prog", "-p",    "65535", "-x", "10", "-y", "10",
                          "-n",   "TeamA", "-c",    "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().port, 65535);
}

TEST(ServerArgsTest, PortTooHigh)
{
    const char* argv[] = {"prog", "-p",    "70000", "-x", "10", "-y", "10",
                          "-n",   "TeamA", "-c",    "5",  "-f", "100"};
    Args args(13, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}
