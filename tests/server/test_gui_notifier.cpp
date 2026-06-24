#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game/GuiNotifier.hpp"
#include "network/ClientManager.hpp"
#include "network/Listener.hpp"

// Opens a raw TCP socket and connects it to localhost:port.
// Returns the fd, or -1 on failure. Caller must close().
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

// Accepts one incoming connection into cm and returns its connection id.
static int acceptOne(ClientManager& cm)
{
    auto result = cm.poll(200);
    EXPECT_EQ(result.newConnections.size(), 1u);
    return result.newConnections.empty() ? -1 : result.newConnections[0];
}

TEST(GuiNotifier, BroadcastWithNoGuisIsNoOp)
{
    Listener l(14260);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    EXPECT_NO_THROW(notifier.broadcast("msz 10 10\n"));
}

TEST(GuiNotifier, BroadcastReachesRegisteredGui)
{
    Listener l(14261);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int clientFd = connectTo(14261);
    ASSERT_GE(clientFd, 0);

    int id = acceptOne(cm);
    ASSERT_GE(id, 0);

    notifier.addGui(id);
    notifier.broadcast("msz 10 10\n");

    EXPECT_TRUE(cm.getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST(GuiNotifier, BroadcastSkipsNonGuiConnection)
{
    Listener l(14262);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int clientFd = connectTo(14262);
    ASSERT_GE(clientFd, 0);

    int id = acceptOne(cm);
    ASSERT_GE(id, 0);

    // id is never passed to addGui — it stays as a non-GUI connection
    notifier.broadcast("msz 10 10\n");

    EXPECT_FALSE(cm.getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST(GuiNotifier, BroadcastReachesAllGuisNotJustOne)
{
    Listener l(14263);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int fd1 = connectTo(14263);
    ASSERT_GE(fd1, 0);
    int id1 = acceptOne(cm);
    ASSERT_GE(id1, 0);

    int fd2 = connectTo(14263);
    ASSERT_GE(fd2, 0);
    int id2 = acceptOne(cm);
    ASSERT_GE(id2, 0);

    notifier.addGui(id1);
    notifier.addGui(id2);
    notifier.broadcast("tna TeamA\n");

    EXPECT_TRUE(cm.getConnection(id1).hasPendingWrite());
    EXPECT_TRUE(cm.getConnection(id2).hasPendingWrite());

    close(fd1);
    close(fd2);
}

TEST(GuiNotifier, BroadcastOnlyReachesGuisNotMixedConnections)
{
    Listener l(14264);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int fdGui = connectTo(14264);
    ASSERT_GE(fdGui, 0);
    int guiId = acceptOne(cm);
    ASSERT_GE(guiId, 0);

    int fdAi = connectTo(14264);
    ASSERT_GE(fdAi, 0);
    int aiId = acceptOne(cm);
    ASSERT_GE(aiId, 0);

    notifier.addGui(guiId);
    // aiId intentionally not added
    notifier.broadcast("pdi #3\n");

    EXPECT_TRUE(cm.getConnection(guiId).hasPendingWrite());
    EXPECT_FALSE(cm.getConnection(aiId).hasPendingWrite());

    close(fdGui);
    close(fdAi);
}

TEST(GuiNotifier, RemoveGuiStopsBroadcast)
{
    Listener l(14265);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int clientFd = connectTo(14265);
    ASSERT_GE(clientFd, 0);

    int id = acceptOne(cm);
    ASSERT_GE(id, 0);

    notifier.addGui(id);
    notifier.removeGui(id);
    notifier.broadcast("msz 10 10\n");

    EXPECT_FALSE(cm.getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST(GuiNotifier, SendTargetsOneConnection)
{
    Listener l(14266);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int fd1 = connectTo(14266);
    ASSERT_GE(fd1, 0);
    int id1 = acceptOne(cm);
    ASSERT_GE(id1, 0);

    int fd2 = connectTo(14266);
    ASSERT_GE(fd2, 0);
    int id2 = acceptOne(cm);
    ASSERT_GE(id2, 0);

    notifier.send(id1, "suc\n");

    EXPECT_TRUE(cm.getConnection(id1).hasPendingWrite());
    EXPECT_FALSE(cm.getConnection(id2).hasPendingWrite());

    close(fd1);
    close(fd2);
}

TEST(GuiNotifier, OnPlayerAddedBroadcastsPnw)
{
    Listener l(14267);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int clientFd = connectTo(14267);
    ASSERT_GE(clientFd, 0);
    int id = acceptOne(cm);
    ASSERT_GE(id, 0);

    notifier.addGui(id);
    notifier.onPlayerAdded(7, 2, 3, Orientation::N, 1, "TeamA");

    EXPECT_TRUE(cm.getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST(GuiNotifier, OnPlayerRemovedBroadcastsPdi)
{
    Listener l(14268);
    ClientManager cm(l);
    GuiNotifier notifier(cm);

    int clientFd = connectTo(14268);
    ASSERT_GE(clientFd, 0);
    int id = acceptOne(cm);
    ASSERT_GE(id, 0);

    notifier.addGui(id);
    notifier.onPlayerRemoved(5);

    EXPECT_TRUE(cm.getConnection(id).hasPendingWrite());

    close(clientFd);
}
