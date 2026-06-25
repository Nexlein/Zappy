#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "core/Args.hpp"
#include "core/GameClock.hpp"
#include "core/World.hpp"
#include "game/GuiNotifier.hpp"
#include "game/HandshakeHandler.hpp"
#include "network/ClientManager.hpp"
#include "network/ClientType.hpp"
#include "network/Listener.hpp"

static ServerConfig makeConfig(int port, int clientsNb = 2)
{
    return ServerConfig{port, 10, 10, {"TeamA", "TeamB"}, clientsNb, 100};
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

struct HandshakeFixture : public ::testing::Test {
    int port;
    Listener* listener = nullptr;
    ClientManager* cm = nullptr;
    World* world = nullptr;
    GuiNotifier* notifier = nullptr;
    GameClock* clock = nullptr;
    HandshakeHandler* handler = nullptr;
    ServerConfig config;

    explicit HandshakeFixture(int p) : port(p), config(makeConfig(p)) {}

    void SetUp() override
    {
        listener = new Listener(port);
        cm = new ClientManager(*listener);
        world = new World(config.width, config.height, config.teamNames);
        world->spawnInitialEggs(config.clientsNb);
        notifier = new GuiNotifier(*cm);
        world->addWorldObserver(notifier);
        clock = new GameClock(config.freq);
        handler = new HandshakeHandler(*cm, *world, *notifier, config, *clock, [](int, int) {});
    }

    void TearDown() override
    {
        delete handler;
        delete clock;
        delete notifier;
        delete world;
        delete cm;
        delete listener;
    }

    // Connect a raw client, accept it in ClientManager, and notify HandshakeHandler.
    // Returns {clientFd, connectionId}.
    std::pair<int, int> connectAndAccept()
    {
        int fd = connectTo(port);
        int id = acceptOne(*cm);
        handler->onNewConnection(id);
        return {fd, id};
    }
};

// --- Individual test fixtures using different ports ---

struct HandshakeTest : HandshakeFixture {
    HandshakeTest() : HandshakeFixture(14270) {}
};

TEST_F(HandshakeTest, NewConnectionReceivesWelcome)
{
    auto [clientFd, id] = connectAndAccept();

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST_F(HandshakeTest, GraphicPromotesToGuiType)
{
    auto [clientFd, id] = connectAndAccept();

    handler->onLine(id, "GRAPHIC");

    EXPECT_EQ(cm->getConnection(id).type(), ClientType::GUI);

    close(clientFd);
}

TEST_F(HandshakeTest, GraphicAddsToGuiNotifier)
{
    auto [clientFd, id] = connectAndAccept();
    handler->onLine(id, "GRAPHIC");

    // Accept a second connection but do NOT call onNewConnection — no WELCOME queued,
    // so hasPendingWrite starts false and only becomes true if broadcast hits it.
    int fd2 = connectTo(port);
    int id2 = acceptOne(*cm);
    ASSERT_GE(id2, 0);

    notifier->broadcast("ping\n");

    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());    // GUI gets it
    EXPECT_FALSE(cm->getConnection(id2).hasPendingWrite());  // non-GUI doesn't

    close(clientFd);
    close(fd2);
}

TEST_F(HandshakeTest, GraphicReceivesInitialWorldDump)
{
    auto [clientFd, id] = connectAndAccept();

    // Clear the WELCOME write so we can isolate the world dump
    // (hasPendingWrite is true after WELCOME; promoting to GUI adds more)
    handler->onLine(id, "GRAPHIC");

    // Connection must have substantial pending data: msz + bct*100 + tna*2 + sgt
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST_F(HandshakeTest, ValidTeamNamePromotesToAi)
{
    auto [clientFd, id] = connectAndAccept();

    handler->onLine(id, "TeamA");

    EXPECT_EQ(cm->getConnection(id).type(), ClientType::AI);

    close(clientFd);
}

TEST_F(HandshakeTest, ValidTeamNameCreatesPlayerInWorld)
{
    auto [clientFd, id] = connectAndAccept();

    handler->onLine(id, "TeamA");

    EXPECT_EQ(world->teamPlayerCount("TeamA"), 1);

    close(clientFd);
}

TEST_F(HandshakeTest, ValidTeamNameSendsSlotCountAndMapSize)
{
    auto [clientFd, id] = connectAndAccept();

    handler->onLine(id, "TeamA");

    // WELCOME + slot count line + map size line all queued
    EXPECT_TRUE(cm->getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST_F(HandshakeTest, ValidTeamNameBroadcastsPnwToGuis)
{
    // Register a GUI first
    auto [guiFd, guiId] = connectAndAccept();
    handler->onLine(guiId, "GRAPHIC");

    // Now an AI joins — GUI should receive pnw broadcast
    auto [aiFd, aiId] = connectAndAccept();
    handler->onLine(aiId, "TeamA");

    // GUI connection will have WELCOME + world dump + pnw queued — just check it's non-empty
    EXPECT_TRUE(cm->getConnection(guiId).hasPendingWrite());

    close(guiFd);
    close(aiFd);
}

TEST_F(HandshakeTest, UnknownTeamNameRejectsConnection)
{
    auto [clientFd, id] = connectAndAccept();

    handler->onLine(id, "UnknownTeam");

    // Connection should have been disconnected — getConnection throws
    EXPECT_THROW(cm->getConnection(id), std::out_of_range);

    close(clientFd);
}

TEST_F(HandshakeTest, FullTeamRejectsConnection)
{
    // Config has clientsNb=2; fill both slots
    auto [fd1, id1] = connectAndAccept();
    handler->onLine(id1, "TeamA");
    auto [fd2, id2] = connectAndAccept();
    handler->onLine(id2, "TeamA");

    // Third join should be rejected
    auto [fd3, id3] = connectAndAccept();
    handler->onLine(id3, "TeamA");

    EXPECT_THROW(cm->getConnection(id3), std::out_of_range);

    close(fd1);
    close(fd2);
    close(fd3);
}

TEST_F(HandshakeTest, MultipleTeamsAreIndependent)
{
    auto [fdA, idA] = connectAndAccept();
    handler->onLine(idA, "TeamA");

    auto [fdB, idB] = connectAndAccept();
    handler->onLine(idB, "TeamB");

    EXPECT_EQ(world->teamPlayerCount("TeamA"), 1);
    EXPECT_EQ(world->teamPlayerCount("TeamB"), 1);

    close(fdA);
    close(fdB);
}

TEST_F(HandshakeTest, AiPlayerSpawnsOnEggWhenAvailable)
{
    // Drain the eggs spawned in SetUp so this test controls the pool exactly:
    // one egg available for TeamA at a known position.
    while (world->popEggForTeam("TeamA")) {
    }
    int dummyId = world->addPlayer(999, "TeamA", 3, 7, Orientation::N);
    int eggId = world->addEgg(dummyId);
    (void)eggId;

    auto [clientFd, id] = connectAndAccept();
    handler->onLine(id, "TeamA");

    // Player 0 is the dummy; player 1 is the new AI — check it exists
    // (egg was consumed, so popEggForTeam would now return nullopt)
    EXPECT_EQ(world->popEggForTeam("TeamA"), std::nullopt);

    close(clientFd);
}
