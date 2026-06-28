#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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
    return ServerConfig{port, 10, 10, {"TeamA", "TeamB"}, 5, HIGH_FREQ, 42};
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

struct GuiHandlersFixture : public ::testing::Test {
    int port;
    Listener* listener = nullptr;
    ClientManager* cm = nullptr;
    World* world = nullptr;
    GuiNotifier* notifier = nullptr;
    Scheduler* scheduler = nullptr;
    CommandDispatcher* dispatcher = nullptr;
    ServerConfig config;

    explicit GuiHandlersFixture(int p) : port(p), config(makeConfig(p)) {}

    void SetUp() override
    {
        listener = new Listener(port);
        cm = new ClientManager(*listener);
        world = new World(config.width, config.height, config.teamNames, 42);
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

    std::pair<int, int> connectAsGui()
    {
        int fd = connectTo(port);
        int id = acceptOne(*cm);
        dispatcher->onNewConnection(id);
        dispatcher->dispatch(id, "GRAPHIC");
        return {fd, id};
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

struct GuiHandlersTest : GuiHandlersFixture {
    GuiHandlersTest() : GuiHandlersFixture(14290) {}
};

// --- msz ---

TEST_F(GuiHandlersTest, MszQueuesResponse)
{
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "msz");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- bct ---

TEST_F(GuiHandlersTest, BctValidCoordsQueuesResponse)
{
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "bct 0 0");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

TEST_F(GuiHandlersTest, BctToroidalCoordsAreAccepted)
{
    // bct uses at() which wraps — out-of-range coords wrap silently
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "bct 99 99");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- mct ---

TEST_F(GuiHandlersTest, MctQueuesOneEntryPerTile)
{
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "mct");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- tna ---

TEST_F(GuiHandlersTest, TnaQueuesOneEntryPerTeam)
{
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "tna");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- ppo ---

TEST_F(GuiHandlersTest, PpoKnownPlayerQueuesResponse)
{
    auto [aiFd, aiId] = connectAsAi();
    int pid = cm->getConnection(aiId).playerId();

    auto [guiFd, guiId] = connectAsGui();
    dispatcher->dispatch(guiId, "ppo #" + std::to_string(pid));
    EXPECT_TRUE(cm->getConnection(guiId).hasPendingWrite());

    close(aiFd);
    close(guiFd);
}

TEST_F(GuiHandlersTest, PpoUnknownPlayerSendsSuc)
{
    auto [fd, id] = connectAsGui();
    EXPECT_NO_THROW(dispatcher->dispatch(id, "ppo #9999"));
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- plv ---

TEST_F(GuiHandlersTest, PlvKnownPlayerQueuesResponse)
{
    auto [aiFd, aiId] = connectAsAi();
    int pid = cm->getConnection(aiId).playerId();

    auto [guiFd, guiId] = connectAsGui();
    dispatcher->dispatch(guiId, "plv #" + std::to_string(pid));
    EXPECT_TRUE(cm->getConnection(guiId).hasPendingWrite());

    close(aiFd);
    close(guiFd);
}

TEST_F(GuiHandlersTest, PlvUnknownPlayerSendsSuc)
{
    auto [fd, id] = connectAsGui();
    EXPECT_NO_THROW(dispatcher->dispatch(id, "plv #9999"));
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- pin ---

TEST_F(GuiHandlersTest, PinKnownPlayerQueuesResponse)
{
    auto [aiFd, aiId] = connectAsAi();
    int pid = cm->getConnection(aiId).playerId();

    auto [guiFd, guiId] = connectAsGui();
    dispatcher->dispatch(guiId, "pin #" + std::to_string(pid));
    EXPECT_TRUE(cm->getConnection(guiId).hasPendingWrite());

    close(aiFd);
    close(guiFd);
}

TEST_F(GuiHandlersTest, PinUnknownPlayerSendsSuc)
{
    auto [fd, id] = connectAsGui();
    EXPECT_NO_THROW(dispatcher->dispatch(id, "pin #9999"));
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- sgt ---

TEST_F(GuiHandlersTest, SgtQueuesCurrentFreq)
{
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "sgt");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}

// --- sst ---

TEST_F(GuiHandlersTest, SstBroadcastsToAllGuis)
{
    auto [fd1, id1] = connectAsGui();
    auto [fd2, id2] = connectAsGui();

    dispatcher->dispatch(id1, "sst 200");

    // Both GUI connections should have the sst broadcast queued
    EXPECT_TRUE(cm->getConnection(id1).hasPendingWrite());
    EXPECT_TRUE(cm->getConnection(id2).hasPendingWrite());

    close(fd1);
    close(fd2);
}

TEST_F(GuiHandlersTest, SstAffectsSubsequentAiCommandTiming)
{
    auto [guiFd, guiId] = connectAsGui();
    auto [aiFd, aiId] = connectAsAi();

    // Switch to a low freq so Forward delay becomes > 0ms
    dispatcher->dispatch(guiId, "sst 1");
    // Now Forward delay = 7000 / 1 = 7000ms — scheduler won't fire immediately
    dispatcher->dispatch(aiId, "Forward");

    EXPECT_GT(scheduler->msUntilNext(), 0);

    close(guiFd);
    close(aiFd);
}

TEST_F(GuiHandlersTest, SstRescalesShrinksPendingDelays)
{
    auto [guiFd, guiId] = connectAsGui();
    auto [aiFd, aiId] = connectAsAi();

    // Schedule a Forward at freq=1 (7000ms delay)
    dispatcher->dispatch(guiId, "sst 1");
    dispatcher->dispatch(aiId, "Forward");
    int delayBefore = scheduler->msUntilNext();

    // Double the freq — pending delay should halve
    dispatcher->dispatch(guiId, "sst 2");
    int delayAfter = scheduler->msUntilNext();

    EXPECT_LT(delayAfter, delayBefore);

    close(guiFd);
    close(aiFd);
}

// --- team join tracking ---

TEST_F(GuiHandlersTest, TeamJoinStampedOnFirstAiJoin)
{
    EXPECT_FALSE(dispatcher->teamJoin("TeamA").has_value());

    auto [fd, id] = connectAsAi("TeamA");
    EXPECT_TRUE(dispatcher->teamJoin("TeamA").has_value());
    EXPECT_FALSE(dispatcher->teamJoin("TeamB").has_value());
    close(fd);
}

// --- unknown command ---

TEST_F(GuiHandlersTest, UnknownCommandQueuesSuc)
{
    auto [fd, id] = connectAsGui();
    dispatcher->dispatch(id, "nonsense");
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());
    close(fd);
}
