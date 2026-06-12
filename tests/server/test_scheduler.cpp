#include <gtest/gtest.h>

#include "../../server/src/core/Scheduler.hpp"

TEST(SchedulerTest, FiresCallbackAfterDelay)
{
    Scheduler sched;
    bool fired = false;

    sched.schedule(std::chrono::milliseconds(0), [&fired] { fired = true; });
    sched.tick();

    EXPECT_TRUE(fired);
}

TEST(SchedulerTest, DoesNotFireFutureEvent)
{
    Scheduler sched;
    bool fired = false;

    sched.schedule(std::chrono::milliseconds(60000), [&fired] { fired = true; });
    sched.tick();

    EXPECT_FALSE(fired);
}

TEST(SchedulerTest, MsUntilNextNegativeWhenEmpty)
{
    Scheduler sched;
    EXPECT_EQ(sched.msUntilNext(), -1);
}

TEST(SchedulerTest, MsUntilNextNonNegativeWhenPending)
{
    Scheduler sched;
    sched.schedule(std::chrono::milliseconds(60000), [] {});
    EXPECT_GE(sched.msUntilNext(), 0);
}

TEST(SchedulerTest, MultipleEventsFireInOrder)
{
    Scheduler sched;
    std::vector<int> order;

    sched.schedule(std::chrono::milliseconds(0), [&] { order.push_back(1); });
    sched.schedule(std::chrono::milliseconds(0), [&] { order.push_back(2); });
    sched.schedule(std::chrono::milliseconds(0), [&] { order.push_back(3); });

    sched.tick();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST(SchedulerTest, RescaleStretchesDuration)
{
    Scheduler sched;
    bool fired = false;

    sched.schedule(std::chrono::milliseconds(100), [&] { fired = true; });
    sched.rescale(100.0f);
    sched.tick();

    EXPECT_FALSE(fired);
}

TEST(SchedulerTest, CallbackCanReschedule)
{
    Scheduler sched;
    int count = 0;

    sched.schedule(std::chrono::milliseconds(0), [&] {
        count++;
        sched.schedule(std::chrono::milliseconds(0), [&] { count++; });
    });

    sched.tick();
    EXPECT_EQ(count, 1);
    sched.tick();
    EXPECT_EQ(count, 2);
}
