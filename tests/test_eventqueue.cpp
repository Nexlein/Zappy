#include <gtest/gtest.h>
#include "core/EventQueue.hpp"
#include <thread>
#include <vector>

TEST(EventQueueTest, PushPop) {
    EventQueue queue;
    queue.push(MapSize{10, 20});

    auto event = queue.pop();
    ASSERT_TRUE(event.has_value());
    ASSERT_TRUE(std::holds_alternative<MapSize>(*event));
    EXPECT_EQ(std::get<MapSize>(*event).width, 10);
    EXPECT_EQ(std::get<MapSize>(*event).height, 20);
}

TEST(EventQueueTest, PopEmpty) {
    EventQueue queue;
    auto event = queue.pop();
    EXPECT_FALSE(event.has_value());
}

TEST(EventQueueTest, FIFO) {
    EventQueue queue;
    queue.push(MapSize{1, 2});
    queue.push(TeamName{"TeamA"});
    queue.push(MapSize{3, 4});

    auto e1 = queue.pop();
    auto e2 = queue.pop();
    auto e3 = queue.pop();

    ASSERT_TRUE(std::holds_alternative<MapSize>(*e1));
    EXPECT_EQ(std::get<MapSize>(*e1).width, 1);

    ASSERT_TRUE(std::holds_alternative<TeamName>(*e2));
    EXPECT_EQ(std::get<TeamName>(*e2).name, "TeamA");

    ASSERT_TRUE(std::holds_alternative<MapSize>(*e3));
    EXPECT_EQ(std::get<MapSize>(*e3).width, 3);
}

TEST(EventQueueTest, MultipleEvents) {
    EventQueue queue;

    // Push various event types
    queue.push(MapSize{10, 10});
    queue.push(TileContent{0, 0, Resources{5, 1, 2, 3, 4, 5, 6}});
    queue.push(PlayerNew{42, 5, 7, Orientation::E, 1, "TeamA"});
    queue.push(PlayerDeath{42});
    queue.push(GameEnd{"TeamA"});

    // Verify all types preserved
    EXPECT_TRUE(std::holds_alternative<MapSize>(*queue.pop()));
    EXPECT_TRUE(std::holds_alternative<TileContent>(*queue.pop()));
    EXPECT_TRUE(std::holds_alternative<PlayerNew>(*queue.pop()));
    EXPECT_TRUE(std::holds_alternative<PlayerDeath>(*queue.pop()));
    EXPECT_TRUE(std::holds_alternative<GameEnd>(*queue.pop()));

    // Queue empty
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(EventQueueTest, ConcurrentPushPop) {
    EventQueue queue;
    constexpr int NUM_EVENTS = 1000;

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < NUM_EVENTS; ++i) {
            queue.push(MapSize{i, i});
        }
    });

    // Consumer thread
    std::vector<int> consumed;
    std::thread consumer([&]() {
        int count = 0;
        while (count < NUM_EVENTS) {
            if (auto e = queue.pop()) {
                if (auto* ms = std::get_if<MapSize>(&*e)) {
                    consumed.push_back(ms->width);
                    count++;
                }
            }
            // Small yield to avoid busy-wait
            std::this_thread::yield();
        }
    });

    producer.join();
    consumer.join();

    // All events consumed
    EXPECT_EQ(consumed.size(), NUM_EVENTS);

    // Queue empty after consumption
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(EventQueueTest, MultipleProducersOneConsumer) {
    EventQueue queue;
    constexpr int NUM_THREADS = 4;
    constexpr int EVENTS_PER_THREAD = 250;
    constexpr int TOTAL_EVENTS = NUM_THREADS * EVENTS_PER_THREAD;

    std::vector<std::thread> producers;

    // Multiple producer threads
    for (int t = 0; t < NUM_THREADS; ++t) {
        producers.emplace_back([&, t]() {
            for (int i = 0; i < EVENTS_PER_THREAD; ++i) {
                queue.push(MapSize{t, i});
            }
        });
    }

    // Single consumer
    int consumedCount = 0;
    std::thread consumer([&]() {
        while (consumedCount < TOTAL_EVENTS) {
            if (queue.pop().has_value()) {
                consumedCount++;
            }
            std::this_thread::yield();
        }
    });

    for (auto& p : producers) {
        p.join();
    }
    consumer.join();

    EXPECT_EQ(consumedCount, TOTAL_EVENTS);
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(EventQueueTest, OneProducerMultipleConsumers) {
    EventQueue queue;
    constexpr int NUM_EVENTS = 1000;
    constexpr int NUM_CONSUMERS = 4;

    std::atomic<int> totalConsumed{0};

    // Single producer
    std::thread producer([&]() {
        for (int i = 0; i < NUM_EVENTS; ++i) {
            queue.push(MapSize{i, i});
        }
    });

    // Multiple consumers
    std::vector<std::thread> consumers;

    for (int c = 0; c < NUM_CONSUMERS; ++c) {
        consumers.emplace_back([&]() {
            while (totalConsumed.load() < NUM_EVENTS) {
                if (auto e = queue.pop()) {
                    totalConsumed.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    producer.join();
    for (auto& c : consumers) {
        c.join();
    }

    EXPECT_EQ(totalConsumed.load(), NUM_EVENTS);
    EXPECT_FALSE(queue.pop().has_value());
}
