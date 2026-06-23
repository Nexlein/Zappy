#include <gtest/gtest.h>

#include <thread>

#include "../../server/src/core/GameClock.hpp"

namespace {
void sleepMs(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
}  // namespace

TEST(GameClockTest, TicksAccumulateAtFreq)
{
    GameClock clock(100);  // 100 ticks/sec
    sleepMs(100);          // ~10 ticks

    EXPECT_NEAR(clock.ticks(), 10.0, 4.0);
}

TEST(GameClockTest, SetFreqReturnsRatio)
{
    GameClock clock(10);
    EXPECT_FLOAT_EQ(clock.setFreq(20), 0.5f);  // oldFreq / newFreq
}

// The core fix: ticks must integrate freq over time, not multiply total
// elapsed time by the final freq. After an sst the banked ticks stay.
TEST(GameClockTest, TicksSurviveFreqChange)
{
    GameClock clock(100);  // 100 ticks/sec
    sleepMs(100);          // ~10 ticks banked at 100
    clock.setFreq(10);     // slow down to 10 ticks/sec
    sleepMs(100);          // ~1 more tick

    // Total ~11. A naive elapsed*finalFreq would give ~200ms * 10 = ~2.
    EXPECT_NEAR(clock.ticks(), 11.0, 4.0);
}
