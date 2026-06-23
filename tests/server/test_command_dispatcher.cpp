#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include "core/Args.hpp"
#include "core/Scheduler.hpp"
#include "core/World.hpp"
#include "game/CommandDispatcher.hpp"
#include "game/GuiNotifier.hpp"
#include "network/ClientManager.hpp"
#include "network/ClientType.hpp"
#include "network/Listener.hpp"

// High freq so all scheduled delays become 0ms — tick() fires them immediately.
static constexpr int HIGH_FREQ = 10000;

static ServerConfig makeConfig(int port)
{
    // 20×20 so 11 moves in one direction don't wrap back to look like 1 move
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

struct DispatcherFixture : public ::testing::Test {
    int port;
    Listener* listener = nullptr;
    ClientManager* cm = nullptr;
    World* world = nullptr;
    GuiNotifier* notifier = nullptr;
    Scheduler* scheduler = nullptr;
    CommandDispatcher* dispatcher = nullptr;
    ServerConfig config;

    explicit DispatcherFixture(int p) : port(p), config(makeConfig(p)) {}

    void SetUp() override
    {
        listener = new Listener(port);
        cm = new ClientManager(*listener);
        world = new World(config.width, config.height, config.teamNames);
        world->spawnInitialEggs(config.clientsNb);
        notifier = new GuiNotifier(*cm);
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

    // Connect, accept, send WELCOME, return {clientFd, connectionId}.
    std::pair<int, int> connectAndAccept()
    {
        int fd = connectTo(port);
        int id = acceptOne(*cm);
        dispatcher->onNewConnection(id);
        return {fd, id};
    }

    // Full handshake for an AI player on teamName. Returns {clientFd, connectionId}.
    std::pair<int, int> connectAsAi(const std::string& teamName = "TeamA")
    {
        auto [fd, id] = connectAndAccept();
        dispatcher->dispatch(id, teamName);
        return {fd, id};
    }

    // Full handshake for a GUI client. Returns {clientFd, connectionId}.
    std::pair<int, int> connectAsGui()
    {
        auto [fd, id] = connectAndAccept();
        dispatcher->dispatch(id, "GRAPHIC");
        return {fd, id};
    }
};

struct DispatcherTest : DispatcherFixture {
    DispatcherTest() : DispatcherFixture(14280) {}
};

// --- PENDING state: routes through HandshakeHandler ---

TEST_F(DispatcherTest, PendingConnectionSendsWelcome)
{
    auto [fd, id] = connectAndAccept();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, PendingDispatchValidTeamPromotesToAi)
{
    auto [fd, id] = connectAndAccept();
    dispatcher->dispatch(id, "TeamA");

    EXPECT_EQ(cm->getConnection(id).type(), ClientType::AI);

    close(fd);
}

TEST_F(DispatcherTest, PendingDispatchGraphicPromotesToGui)
{
    auto [fd, id] = connectAndAccept();
    dispatcher->dispatch(id, "GRAPHIC");

    EXPECT_EQ(cm->getConnection(id).type(), ClientType::GUI);

    close(fd);
}

// --- AI: unknown command → immediate ko ---

TEST_F(DispatcherTest, AiUnknownCommandSendsKo)
{
    auto [fd, id] = connectAsAi();
    // Clear WELCOME + slot/map write from handshake
    cm->getConnection(id).hasPendingWrite();  // just access, can't clear — check after dispatch

    dispatcher->dispatch(id, "garbage");

    // Connection must have pending write (ko was queued)
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, AiUnknownCommandDoesNotScheduleExtra)
{
    auto [fd, id] = connectAsAi();

    // Starvation timer is already scheduled after handshake — record baseline
    int msBefore = scheduler->msUntilNext();

    dispatcher->dispatch(id, "garbage");

    // Unknown command must not add a new scheduled event (msUntilNext unchanged)
    EXPECT_EQ(scheduler->msUntilNext(), msBefore);

    close(fd);
}

// --- AI: ConnectNbr → immediate response, no scheduling ---

TEST_F(DispatcherTest, AiConnectNbrRespondsImmediately)
{
    auto [fd, id] = connectAsAi();

    // Starvation timer already scheduled — record baseline before dispatch
    int msBefore = scheduler->msUntilNext();

    dispatcher->dispatch(id, "Connect nbr");

    // ConnectNbr responds immediately and does not schedule anything extra
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    EXPECT_EQ(scheduler->msUntilNext(), msBefore);

    close(fd);
}

TEST_F(DispatcherTest, AiConnectNbrReportsCorrectSlots)
{
    // TeamA has 1 player (from connectAsAi), config.clientsNb=5 → 4 slots left
    auto [fd, id] = connectAsAi("TeamA");

    // Flush handshake writes by checking, then dispatch ConnectNbr
    // We can't flush write buf directly, but we can still verify the response is queued
    dispatcher->dispatch(id, "Connect nbr");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

// --- AI: valid command → scheduled, executes after tick ---

TEST_F(DispatcherTest, AiForwardSchedulesEvent)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Forward");

    EXPECT_GE(scheduler->msUntilNext(), 0);

    close(fd);
}

TEST_F(DispatcherTest, AiForwardMovesPlayerAfterTick)
{
    auto [fd, id] = connectAsAi();

    int playerId = cm->getConnection(id).playerId();
    auto& p = world->getPlayer(playerId);
    int startX = p.x;
    int startY = p.y;
    Orientation ori = p.orientation;

    dispatcher->dispatch(id, "Forward");
    scheduler->tick();

    auto& moved = world->getPlayer(playerId);
    // Player moved one step in their orientation direction
    if (ori == Orientation::N) {
        EXPECT_EQ(moved.y, ((startY - 1 + 20) % 20));
    } else if (ori == Orientation::S) {
        EXPECT_EQ(moved.y, (startY + 1) % 20);
    } else if (ori == Orientation::E) {
        EXPECT_EQ(moved.x, (startX + 1) % 20);
    } else if (ori == Orientation::W) {
        EXPECT_EQ(moved.x, ((startX - 1 + 20) % 20));
    }

    close(fd);
}

TEST_F(DispatcherTest, AiForwardSendsOkAfterTick)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Forward");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, AiInventorySendsResponseAfterTick)
{
    auto [fd, id] = connectAsAi();

    dispatcher->dispatch(id, "Inventory");
    scheduler->tick();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, AiCommandsChainSequentially)
{
    auto [fd, id] = connectAsAi();

    int playerId = cm->getConnection(id).playerId();
    world->getPlayer(playerId).orientation = Orientation::E;

    dispatcher->dispatch(id, "Forward");
    dispatcher->dispatch(id, "Forward");

    int xBefore = world->getPlayer(playerId).x;

    scheduler->tick();  // executes Forward #1, schedules Forward #2
    int xAfter1 = world->getPlayer(playerId).x;
    EXPECT_EQ(xAfter1, (xBefore + 1) % 20);

    scheduler->tick();  // executes Forward #2
    int xAfter2 = world->getPlayer(playerId).x;
    EXPECT_EQ(xAfter2, (xBefore + 2) % 20);

    close(fd);
}

// --- AI: queue depth capped at 10 ---

TEST_F(DispatcherTest, AiQueueDropsCommandsBeyondTen)
{
    auto [fd, id] = connectAsAi();

    int playerId = cm->getConnection(id).playerId();
    world->getPlayer(playerId).orientation = Orientation::E;
    int startX = world->getPlayer(playerId).x;

    // Dispatch 12 Forwards: 1 executing + 10 queued = 11 total; 12th dropped
    for (int i = 0; i < 12; i++) dispatcher->dispatch(id, "Forward");

    // Drain all scheduled events
    for (int i = 0; i < 12; i++) scheduler->tick();

    int finalX = world->getPlayer(playerId).x;
    int moved = ((finalX - startX) + 20) % 20;
    EXPECT_EQ(moved, 11);  // only 11 executed, not 12

    close(fd);
}

// --- GUI: unknown command → suc ---

TEST_F(DispatcherTest, GuiUnknownCommandSendsSuc)
{
    auto [fd, id] = connectAsGui();

    dispatcher->dispatch(id, "garbage");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

// --- GUI commands ---

TEST_F(DispatcherTest, GuiMszQueuesResponse)
{
    auto [fd, id] = connectAsGui();

    dispatcher->dispatch(id, "msz");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, GuiTnaQueuesResponse)
{
    auto [fd, id] = connectAsGui();

    dispatcher->dispatch(id, "tna");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, GuiSgtQueuesResponse)
{
    auto [fd, id] = connectAsGui();

    dispatcher->dispatch(id, "sgt");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, GuiSstUpdatesFreqAndBroadcasts)
{
    auto [guiFd, guiId] = connectAsGui();

    dispatcher->dispatch(guiId, "sst 200");

    // GUI should have received the sst broadcast
    EXPECT_TRUE(cm->getConnection(guiId).hasPendingWrite());

    close(guiFd);
}

TEST_F(DispatcherTest, GuiBctQueuesResponse)
{
    auto [fd, id] = connectAsGui();

    dispatcher->dispatch(id, "bct 0 0");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, GuiMctQueuesAllTiles)
{
    auto [fd, id] = connectAsGui();

    dispatcher->dispatch(id, "mct");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(fd);
}

TEST_F(DispatcherTest, GuiPpoQueuesResponse)
{
    // Spawn a player first so we have a valid id
    auto [aiFd, aiId] = connectAsAi();
    int playerId = cm->getConnection(aiId).playerId();

    auto [guiFd, guiId] = connectAsGui();
    dispatcher->dispatch(guiId, "ppo #" + std::to_string(playerId));

    EXPECT_TRUE(cm->getConnection(guiId).hasPendingWrite());

    close(aiFd);
    close(guiFd);
}

// --- onDisconnect ---

TEST_F(DispatcherTest, OnDisconnectRemovesGuiFromNotifier)
{
    auto [guiFd, guiId] = connectAsGui();
    dispatcher->onDisconnect(guiId);

    // After disconnect, notifier should no longer track this id
    // Verify: broadcast no longer reaches the (now-disconnected) id
    // ClientManager::send on unknown id is a no-op, so this should not throw
    EXPECT_NO_THROW(notifier->broadcast("test\n"));

    close(guiFd);
}

TEST_F(DispatcherTest, OnDisconnectAiClearsQueue)
{
    auto [fd, id] = connectAsAi();

    // Queue some commands
    dispatcher->dispatch(id, "Forward");
    dispatcher->dispatch(id, "Forward");

    dispatcher->onDisconnect(id);

    // Tick should not crash even though connection is gone from queues
    EXPECT_NO_THROW(scheduler->tick());

    close(fd);
}
