#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "core/Args.hpp"
#include "core/Scheduler.hpp"
#include "core/World.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/GuiNotifier.hpp"
#include "network/ClientManager.hpp"
#include "network/Listener.hpp"

static constexpr int HIGH_FREQ = 10000;

static ServerConfig makeConfig(int port)
{
    return ServerConfig{port, 20, 20, {"TeamA", "TeamB"}, 5, HIGH_FREQ};
}

static int connectTo(int port)
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int acceptOne(ClientManager& cm)
{
    auto result = cm.poll(200);
    if (result.newConnections.empty()) return -1;
    return result.newConnections[0];
}

// --- Fixture ---

struct AiHandlersFixture : public ::testing::Test {
    int port;
    Listener* listener = nullptr;
    ClientManager* cm = nullptr;
    World* world = nullptr;
    GuiNotifier* notifier = nullptr;
    Scheduler* scheduler = nullptr;
    CommandDispatcher* dispatcher = nullptr;
    ServerConfig config;

    explicit AiHandlersFixture(int p) : port(p), config(makeConfig(p)) {}

    void SetUp() override
    {
        listener = new Listener(port);
        cm = new ClientManager(*listener);
        world = new World(config.width, config.height, config.teamNames);
        notifier = new GuiNotifier(*cm);
        world->addWorldObserver(notifier);
        world->spawnInitialEggs(config.clientsNb);
        scheduler = new Scheduler();
        dispatcher = new CommandDispatcher(*cm, *world, *notifier, config, *scheduler);
    }

    void TearDown() override
    {
        delete dispatcher;
        delete scheduler;
        delete notifier;
        delete world;
        delete cm;
        delete listener;
    }

    std::pair<int, int> connectAsAi(const std::string& team = "TeamA")
    {
        int fd = connectTo(port);
        int id = acceptOne(*cm);
        dispatcher->onNewConnection(id);
        dispatcher->dispatch(id, team);
        return {fd, id};
    }
};

struct AiHandlersTest : AiHandlersFixture {
    AiHandlersTest() : AiHandlersFixture(14300) {}
};

// --- Forward ---

TEST_F(AiHandlersTest, ForwardMovesPlayerNorth)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    world->getPlayer(pid).orientation = Orientation::N;
    int startY = world->getPlayer(pid).y;

    dispatcher->dispatch(id, "Forward");
    scheduler->tick();

    EXPECT_EQ(world->getPlayer(pid).y, (startY - 1 + 20) % 20);
    close(fd);
}

TEST_F(AiHandlersTest, ForwardSendsOk)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Forward");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- Right / Left ---

TEST_F(AiHandlersTest, RightRotatesOrientation)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    world->getPlayer(pid).orientation = Orientation::N;

    dispatcher->dispatch(id, "Right");
    scheduler->tick();

    EXPECT_EQ(world->getPlayer(pid).orientation, Orientation::E);
    close(fd);
}

TEST_F(AiHandlersTest, LeftRotatesOrientation)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    world->getPlayer(pid).orientation = Orientation::N;

    dispatcher->dispatch(id, "Left");
    scheduler->tick();

    EXPECT_EQ(world->getPlayer(pid).orientation, Orientation::W);
    close(fd);
}

// --- Inventory ---

TEST_F(AiHandlersTest, InventoryReturnsCurrentFood)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    int food = world->getPlayer(pid).inventory.food;

    dispatcher->dispatch(id, "Inventory");
    scheduler->tick();

    // Response is queued — presence confirms the handler ran
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    // Food value unchanged by Inventory command
    EXPECT_EQ(world->getPlayer(pid).inventory.food, food);
    close(fd);
}

// --- Look ---

TEST_F(AiHandlersTest, LookSendsResponse)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Look");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- Take / Set ---

TEST_F(AiHandlersTest, TakeResourceSucceedsWhenOnTile)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    auto& p = world->getPlayer(pid);
    world->at(p.x, p.y).resources[ResourceType::LINEMATE] = 1;

    dispatcher->dispatch(id, "Take linemate");
    scheduler->tick();

    EXPECT_EQ(world->getPlayer(pid).inventory.linemate, 1);
    EXPECT_EQ(world->at(p.x, p.y).resources[ResourceType::LINEMATE], 0);
    close(fd);
}

TEST_F(AiHandlersTest, TakeResourceFailsWhenNoneOnTile)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    auto& p = world->getPlayer(pid);
    world->at(p.x, p.y).resources[ResourceType::LINEMATE] = 0;

    dispatcher->dispatch(id, "Take linemate");
    scheduler->tick();

    // ko queued — player inventory unchanged
    EXPECT_EQ(world->getPlayer(pid).inventory.linemate, 0);
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

TEST_F(AiHandlersTest, SetResourceDropsOnTile)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    auto& p = world->getPlayer(pid);
    world->getPlayer(pid).inventory.linemate = 1;

    dispatcher->dispatch(id, "Set linemate");
    scheduler->tick();

    EXPECT_EQ(world->getPlayer(pid).inventory.linemate, 0);
    EXPECT_EQ(world->at(p.x, p.y).resources[ResourceType::LINEMATE], 1);
    close(fd);
}

// --- Broadcast ---

TEST_F(AiHandlersTest, BroadcastSendsOkToSender)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Broadcast hello");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

TEST_F(AiHandlersTest, BroadcastDeliveredToOtherPlayer)
{
    auto [fd1, id1] = connectAsAi("TeamA");
    auto [fd2, id2] = connectAsAi("TeamB");

    dispatcher->dispatch(id1, "Broadcast hello");
    scheduler->tick();

    // id2 should have received the message
    EXPECT_TRUE(cm->getConnection(id2).hasPendingWrite());

    close(fd1);
    close(fd2);
}

// --- Eject ---

TEST_F(AiHandlersTest, EjectNoPlayersOnTileSendsKo)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Eject");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

TEST_F(AiHandlersTest, EjectMovesOtherPlayerOffTile)
{
    auto [fd1, id1] = connectAsAi("TeamA");
    auto [fd2, id2] = connectAsAi("TeamB");

    int pid1 = cm->getConnection(id1).playerId();
    int pid2 = cm->getConnection(id2).playerId();

    // Put both on the same tile facing same direction
    world->getPlayer(pid1).orientation = Orientation::N;
    world->movePlayer(pid1, 5, 5);
    world->movePlayer(pid2, 5, 5);

    int startY2 = world->getPlayer(pid2).y;

    dispatcher->dispatch(id1, "Eject");
    scheduler->tick();

    // Ejected player moved one step in ejector's direction (N → y-1)
    EXPECT_NE(world->getPlayer(pid2).y, startY2);
    EXPECT_TRUE(cm->getConnection(id1).hasPendingWrite());  // ok
    EXPECT_TRUE(cm->getConnection(id2).hasPendingWrite());  // eject: <dir>

    close(fd1);
    close(fd2);
}

// --- Fork ---

TEST_F(AiHandlersTest, ForkSendsOk)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Fork");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- Incantation ---

TEST_F(AiHandlersTest, IncantationFailsWhenConditionsNotMet)
{
    auto [fd, id] = connectAsAi();
    // No linemate on tile — level 1 incantation requires 1 linemate
    int pid = cm->getConnection(id).playerId();
    auto& p = world->getPlayer(pid);
    world->at(p.x, p.y).resources[ResourceType::LINEMATE] = 0;

    dispatcher->dispatch(id, "Incantation");

    // startIncantation fails immediately — ko sent before scheduling
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

TEST_F(AiHandlersTest, IncantationSucceedsAndLevelsUp)
{
    auto [fd, id] = connectAsAi();
    int pid = cm->getConnection(id).playerId();
    auto& p = world->getPlayer(pid);
    world->at(p.x, p.y).resources[ResourceType::LINEMATE] = 1;

    dispatcher->dispatch(id, "Incantation");

    // Incantation delay = 300000 / HIGH_FREQ(10000) = 30ms — must wait for fireAt to pass
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    scheduler->tick();

    EXPECT_EQ(world->getPlayer(pid).level, 2);
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}
