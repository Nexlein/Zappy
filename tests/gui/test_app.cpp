#include <gtest/gtest.h>

#include "core/App.hpp"
#include "core/EventQueue.hpp"

// TestableApp exposes App internals without modifying App.hpp
class TestableApp : public App {
    public:
    using App::App;

    GameState& getState() { return state; }
};

// Test state updates through event processing
TEST(AppTest, StateUpdatesFromEvents)
{
    const char* argv[] = {"prog", "-p", "4242"};
    TestableApp app(3, const_cast<char**>(argv));

    EventQueue queue;

    // Manually push events
    queue.push(MapSize{10, 20});
    queue.push(TeamName{"TeamA"});
    queue.push(PlayerNew{1, 5, 7, Orientation::N, 1, "TeamA"});

    // Process events (simulate what run() does)
    while (auto event = queue.pop()) {
        app.getState().applyEvent(*event);
    }

    // Verify state
    GameState& state = app.getState();
    EXPECT_EQ(state.world.width, 10);
    EXPECT_EQ(state.world.height, 20);
    ASSERT_EQ(state.world.teams.size(), 1);
    EXPECT_EQ(state.world.teams[0], "TeamA");
    EXPECT_TRUE(state.world.players.contains(1));
}

// Test event ordering
TEST(AppTest, EventOrderPreserved)
{
    const char* argv[] = {"prog", "-p", "4242"};
    TestableApp app(3, const_cast<char**>(argv));

    EventQueue queue;

    // Push events in specific order
    queue.push(MapSize{5, 5});
    queue.push(TeamName{"Red"});
    queue.push(TeamName{"Blue"});

    // Process in order
    while (auto event = queue.pop()) {
        app.getState().applyEvent(*event);
    }

    GameState& state = app.getState();
    ASSERT_EQ(state.world.teams.size(), 2);
    EXPECT_EQ(state.world.teams[0], "Red");
    EXPECT_EQ(state.world.teams[1], "Blue");
}
