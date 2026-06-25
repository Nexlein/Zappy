#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "interfaces/INetworkObserver.hpp"
#include "network/ClientManager.hpp"
#include "network/Listener.hpp"

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

// --- error cases on empty manager ---

TEST(ClientManager, GetConnectionThrowsOnUnknownId)
{
    Listener l(14250);
    ClientManager cm(l);
    EXPECT_THROW(cm.getConnection(99), std::out_of_range);
}

TEST(ClientManager, DisconnectUnknownIdIsNoOp)
{
    Listener l(14251);
    ClientManager cm(l);
    EXPECT_NO_THROW(cm.disconnect(99));
}

TEST(ClientManager, SendUnknownIdIsNoOp)
{
    Listener l(14252);
    ClientManager cm(l);
    EXPECT_NO_THROW(cm.send(99, "hello\n"));
}

// --- accepting real connections ---

TEST(ClientManager, PollAcceptsIncomingConnection)
{
    Listener l(14253);
    ClientManager cm(l);

    int clientFd = connectTo(14253);
    ASSERT_GE(clientFd, 0);

    auto result = cm.poll(200);

    EXPECT_EQ(result.newConnections.size(), 1u);
    close(clientFd);
}

TEST(ClientManager, AcceptedConnectionIsRetrievable)
{
    Listener l(14254);
    ClientManager cm(l);

    int clientFd = connectTo(14254);
    ASSERT_GE(clientFd, 0);

    auto result = cm.poll(200);
    ASSERT_EQ(result.newConnections.size(), 1u);
    int id = result.newConnections[0];

    EXPECT_NO_THROW(cm.getConnection(id));

    close(clientFd);
}

TEST(ClientManager, AcceptedConnectionHasDefaultTypePending)
{
    Listener l(14255);
    ClientManager cm(l);

    int clientFd = connectTo(14255);
    ASSERT_GE(clientFd, 0);

    auto result = cm.poll(200);
    ASSERT_EQ(result.newConnections.size(), 1u);

    EXPECT_EQ(cm.getConnection(result.newConnections[0]).type(), ClientType::PENDING);

    close(clientFd);
}

TEST(ClientManager, MultipleConnectionsAcceptedSequentially)
{
    Listener l(14256);
    ClientManager cm(l);

    int fd1 = connectTo(14256);
    ASSERT_GE(fd1, 0);
    auto r1 = cm.poll(200);
    ASSERT_EQ(r1.newConnections.size(), 1u);
    int id1 = r1.newConnections[0];

    int fd2 = connectTo(14256);
    ASSERT_GE(fd2, 0);
    auto r2 = cm.poll(200);
    ASSERT_EQ(r2.newConnections.size(), 1u);
    int id2 = r2.newConnections[0];

    EXPECT_NE(id1, id2);
    EXPECT_NO_THROW(cm.getConnection(id1));
    EXPECT_NO_THROW(cm.getConnection(id2));

    close(fd1);
    close(fd2);
}

// --- send queues data into write buffer ---

TEST(ClientManager, SendQueuesPendingWrite)
{
    Listener l(14257);
    ClientManager cm(l);

    int clientFd = connectTo(14257);
    ASSERT_GE(clientFd, 0);

    auto result = cm.poll(200);
    ASSERT_EQ(result.newConnections.size(), 1u);
    int id = result.newConnections[0];

    cm.send(id, "hello\n");

    EXPECT_TRUE(cm.getConnection(id).hasPendingWrite());

    close(clientFd);
}

TEST(ClientManager, SendDataReachesClient)
{
    Listener l(14258);
    ClientManager cm(l);

    int clientFd = connectTo(14258);
    ASSERT_GE(clientFd, 0);

    auto result = cm.poll(200);
    ASSERT_EQ(result.newConnections.size(), 1u);
    int id = result.newConnections[0];

    cm.send(id, "hello\n");

    // Poll again with POLLOUT ready — flushWrite is called
    cm.poll(200);

    // Read on the client side to confirm bytes arrived
    struct timeval tv {
        1, 0
    };
    setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char buf[64] = {};
    ssize_t n = ::recv(clientFd, buf, sizeof(buf) - 1, 0);

    EXPECT_GT(n, 0);
    EXPECT_STREQ(buf, "hello\n");

    close(clientFd);
}

// --- disconnect ---

TEST(ClientManager, DisconnectRemovesConnection)
{
    Listener l(14259);
    ClientManager cm(l);

    int clientFd = connectTo(14259);
    ASSERT_GE(clientFd, 0);

    auto result = cm.poll(200);
    ASSERT_EQ(result.newConnections.size(), 1u);
    int id = result.newConnections[0];

    cm.disconnect(id);

    EXPECT_THROW(cm.getConnection(id), std::out_of_range);

    close(clientFd);
}

TEST(ClientManager, DisconnectDoesNotAffectOtherConnections)
{
    Listener l(14260);
    ClientManager cm(l);

    int fd1 = connectTo(14260);
    ASSERT_GE(fd1, 0);
    int id1 = cm.poll(200).newConnections[0];

    int fd2 = connectTo(14260);
    ASSERT_GE(fd2, 0);
    int id2 = cm.poll(200).newConnections[0];

    cm.disconnect(id1);

    EXPECT_THROW(cm.getConnection(id1), std::out_of_range);
    EXPECT_NO_THROW(cm.getConnection(id2));

    close(fd1);
    close(fd2);
}

// --- receiving data ---

TEST(ClientManager, ReceivedLineAppearsInPollResult)
{
    Listener l(14261);
    ClientManager cm(l);

    int clientFd = connectTo(14261);
    ASSERT_GE(clientFd, 0);

    auto accepted = cm.poll(200);
    ASSERT_EQ(accepted.newConnections.size(), 1u);

    // Client sends a line
    const char* msg = "Forward\n";
    ::send(clientFd, msg, strlen(msg), 0);

    auto result = cm.poll(200);

    ASSERT_EQ(result.lines.size(), 1u);
    EXPECT_EQ(result.lines[0].second, "Forward");

    close(clientFd);
}

TEST(ClientManager, ClientDisconnectDetectedOnPoll)
{
    Listener l(14262);
    ClientManager cm(l);

    int clientFd = connectTo(14262);
    ASSERT_GE(clientFd, 0);

    auto accepted = cm.poll(200);
    ASSERT_EQ(accepted.newConnections.size(), 1u);
    int id = accepted.newConnections[0];

    // Client closes — server detects on next poll
    close(clientFd);

    auto result = cm.poll(200);

    // poll() reports the disconnect in disconnectedIds; caller must clean up
    ASSERT_EQ(result.disconnectedIds.size(), 1u);
    EXPECT_EQ(result.disconnectedIds[0], id);

    cm.disconnect(id);
    EXPECT_THROW(cm.getConnection(id), std::out_of_range);
}

// --- INetworkObserver ---

struct SpyObserver : public INetworkObserver {
    std::vector<int> connected;
    std::vector<int> disconnected;
    std::vector<std::pair<int, std::string>> linesReceived;
    std::vector<std::pair<int, std::string>> linesSent;

    void onClientConnected(int id) override { connected.push_back(id); }
    void onClientDisconnected(int id) override { disconnected.push_back(id); }
    void onLineReceived(int id, const std::string& line) override
    {
        linesReceived.emplace_back(id, line);
    }
    void onLineSent(int id, const std::string& line) override { linesSent.emplace_back(id, line); }
};

TEST(ClientManager, ObserverFiredOnConnect)
{
    Listener l(14263);
    ClientManager cm(l);
    SpyObserver spy;
    cm.addNetworkObserver(&spy);

    int clientFd = connectTo(14263);
    ASSERT_GE(clientFd, 0);

    cm.poll(200);

    ASSERT_EQ(spy.connected.size(), 1u);
    EXPECT_GE(spy.connected[0], 0);

    close(clientFd);
}

TEST(ClientManager, ObserverFiredOnDisconnect)
{
    Listener l(14264);
    ClientManager cm(l);
    SpyObserver spy;
    cm.addNetworkObserver(&spy);

    int clientFd = connectTo(14264);
    ASSERT_GE(clientFd, 0);
    cm.poll(200);
    ASSERT_EQ(spy.connected.size(), 1u);
    int id = spy.connected[0];

    cm.disconnect(id);

    ASSERT_EQ(spy.disconnected.size(), 1u);
    EXPECT_EQ(spy.disconnected[0], id);

    close(clientFd);
}

TEST(ClientManager, ObserverFiredOnLineReceived)
{
    Listener l(14265);
    ClientManager cm(l);
    SpyObserver spy;
    cm.addNetworkObserver(&spy);

    int clientFd = connectTo(14265);
    ASSERT_GE(clientFd, 0);
    cm.poll(200);

    const char* msg = "Forward\n";
    ::send(clientFd, msg, strlen(msg), 0);
    cm.poll(200);

    ASSERT_EQ(spy.linesReceived.size(), 1u);
    EXPECT_EQ(spy.linesReceived[0].second, "Forward");

    close(clientFd);
}

TEST(ClientManager, ObserverFiredOnSend)
{
    Listener l(14266);
    ClientManager cm(l);
    SpyObserver spy;
    cm.addNetworkObserver(&spy);

    int clientFd = connectTo(14266);
    ASSERT_GE(clientFd, 0);
    auto result = cm.poll(200);
    ASSERT_EQ(result.newConnections.size(), 1u);
    int id = result.newConnections[0];

    cm.send(id, "WELCOME\n");

    ASSERT_EQ(spy.linesSent.size(), 1u);
    EXPECT_EQ(spy.linesSent[0].first, id);
    EXPECT_EQ(spy.linesSent[0].second, "WELCOME\n");

    close(clientFd);
}
