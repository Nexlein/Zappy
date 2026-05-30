#include <gtest/gtest.h>
#include "core/Args.hpp"

// Valid arguments
TEST(ArgsTest, ValidPortOnly) {
    const char* argv[] = {"prog", "-p", "4242"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_FALSE(args.isHelpRequested());
    EXPECT_EQ(args.exitCode(), 0);

    auto config = args.getConfig();
    EXPECT_EQ(config.port, 4242);
    EXPECT_EQ(config.machine, "localhost");
}

TEST(ArgsTest, ValidPortAndMachine) {
    const char* argv[] = {"prog", "-p", "8080", "-h", "example.com"};
    Args args(5, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_FALSE(args.isHelpRequested());

    auto config = args.getConfig();
    EXPECT_EQ(config.port, 8080);
    EXPECT_EQ(config.machine, "example.com");
    EXPECT_FALSE(config.headless);
}

TEST(ArgsTest, ValidWithHeadless) {
    const char* argv[] = {"prog", "-p", "4242", "--headless"};
    Args args(4, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());

    auto config = args.getConfig();
    EXPECT_EQ(config.port, 4242);
    EXPECT_TRUE(config.headless);
}

TEST(ArgsTest, ValidAllFlags) {
    const char* argv[] = {"prog", "-p", "9000", "-h", "server.com", "--headless"};
    Args args(6, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());

    auto config = args.getConfig();
    EXPECT_EQ(config.port, 9000);
    EXPECT_EQ(config.machine, "server.com");
    EXPECT_TRUE(config.headless);
}

TEST(ArgsTest, ValidMachineThenPort) {
    const char* argv[] = {"prog", "-h", "192.168.1.1", "-p", "3000"};
    Args args(5, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());

    auto config = args.getConfig();
    EXPECT_EQ(config.port, 3000);
    EXPECT_EQ(config.machine, "192.168.1.1");
}

// Help requested
TEST(ArgsTest, HelpFlag) {
    const char* argv[] = {"prog", "--help"};
    Args args(2, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_TRUE(args.isHelpRequested());
    EXPECT_EQ(args.exitCode(), 0);
}

TEST(ArgsTest, HelpWithOtherArgs) {
    const char* argv[] = {"prog", "-p", "4242", "--help"};
    Args args(4, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_TRUE(args.isHelpRequested());
    EXPECT_EQ(args.exitCode(), 0);
}

// Missing arguments
TEST(ArgsTest, NoArguments) {
    const char* argv[] = {"prog"};
    Args args(1, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_FALSE(args.isHelpRequested());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, MissingPort) {
    const char* argv[] = {"prog", "-h", "example.com"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, PortFlagWithoutValue) {
    const char* argv[] = {"prog", "-p"};
    Args args(2, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, MachineFlagWithoutValue) {
    const char* argv[] = {"prog", "-p", "4242", "-h"};
    Args args(4, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

// Invalid port values
TEST(ArgsTest, InvalidPortNotANumber) {
    const char* argv[] = {"prog", "-p", "not_a_number"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, InvalidPortNegative) {
    const char* argv[] = {"prog", "-p", "-1"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, InvalidPortZero) {
    const char* argv[] = {"prog", "-p", "0"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, InvalidPortTooLarge) {
    const char* argv[] = {"prog", "-p", "70000"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, ValidPortBoundary1) {
    const char* argv[] = {"prog", "-p", "1"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().port, 1);
}

TEST(ArgsTest, ValidPortBoundary65535) {
    const char* argv[] = {"prog", "-p", "65535"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().port, 65535);
}

// Unknown arguments
TEST(ArgsTest, UnknownFlag) {
    const char* argv[] = {"prog", "-p", "4242", "--unknown"};
    Args args(4, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

TEST(ArgsTest, UnknownShortFlag) {
    const char* argv[] = {"prog", "-x", "value"};
    Args args(3, const_cast<char**>(argv));

    EXPECT_FALSE(args.isValid());
    EXPECT_EQ(args.exitCode(), 84);
}

// Edge cases
TEST(ArgsTest, DuplicatePort) {
    const char* argv[] = {"prog", "-p", "4242", "-p", "8080"};
    Args args(5, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().port, 8080);  // Last one wins
}

TEST(ArgsTest, DuplicateMachine) {
    const char* argv[] = {"prog", "-p", "4242", "-h", "host1", "-h", "host2"};
    Args args(7, const_cast<char**>(argv));

    EXPECT_TRUE(args.isValid());
    EXPECT_EQ(args.getConfig().machine, "host2");  // Last one wins
}
