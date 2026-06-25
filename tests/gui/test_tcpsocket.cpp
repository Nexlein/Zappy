#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <thread>

#include "network/TcpSocket.hpp"

/**
 * Mock server for testing TcpSocket.
 * Binds to a port, accepts one connection, sends/receives data.
 */
class MockServer {
    public:
    MockServer(int port) : _port(port), _serverFd(-1), _clientFd(-1) {}

    ~MockServer() { stop(); }

    void start()
    {
        _serverFd = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_NE(_serverFd, -1) << "Failed to create server socket";

        int opt = 1;
        setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(_port);

        ASSERT_NE(bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)), -1)
            << "Failed to bind: " << strerror(errno);
        ASSERT_NE(listen(_serverFd, 1), -1) << "Failed to listen";
    }

    void acceptConnection()
    {
        struct sockaddr_in clientAddr {};
        socklen_t clientLen = sizeof(clientAddr);
        _clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
        ASSERT_NE(_clientFd, -1) << "Failed to accept connection";
    }

    void send(const std::string& data)
    {
        ASSERT_NE(_clientFd, -1) << "No client connected";
        ssize_t sent = ::send(_clientFd, data.c_str(), data.size(), 0);
        ASSERT_EQ(sent, static_cast<ssize_t>(data.size())) << "Failed to send";
    }

    std::string recv(size_t maxLen = 1024)
    {
        if (_clientFd == -1) {
            ADD_FAILURE() << "No client connected";
            return "";
        }
        char buffer[1024];
        ssize_t received = ::recv(_clientFd, buffer, std::min(maxLen, sizeof(buffer)), 0);
        if (received < 0) {
            ADD_FAILURE() << "Failed to recv";
            return "";
        }
        return std::string(buffer, received);
    }

    void stop()
    {
        if (_clientFd != -1) {
            close(_clientFd);
            _clientFd = -1;
        }
        if (_serverFd != -1) {
            close(_serverFd);
            _serverFd = -1;
        }
    }

    private:
    int _port;
    int _serverFd;
    int _clientFd;
};

class TcpSocketTest : public ::testing::Test {
    protected:
    static constexpr int TEST_PORT = 44242;  // High port to avoid conflicts

    void SetUp() override { server = std::make_unique<MockServer>(TEST_PORT); }

    void TearDown() override { server.reset(); }

    std::unique_ptr<MockServer> server;
};

TEST_F(TcpSocketTest, ConnectToServer)
{
    server->start();

    // Accept connection in background thread
    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    EXPECT_NO_THROW(socket.connect("localhost", TEST_PORT));

    acceptThread.join();
}

TEST_F(TcpSocketTest, ConnectToNonexistentServer)
{
    TcpSocket socket;
    EXPECT_THROW(socket.connect("localhost", 44243), TcpException);  // Nothing listening
}

TEST_F(TcpSocketTest, SendData)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    EXPECT_NO_THROW(socket.send("GRAPHIC\n"));

    std::string received = server->recv();
    EXPECT_EQ(received, "GRAPHIC\n");
}

TEST_F(TcpSocketTest, RecvLineCompleteLine)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // Server sends complete line
    server->send("WELCOME\n");

    // Give time for data to arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto line = socket.recvLine();
    ASSERT_TRUE(line.has_value());
    EXPECT_EQ(*line, "WELCOME\n");
}

TEST_F(TcpSocketTest, RecvLinePartialThenComplete)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // Server sends partial line
    server->send("WEL");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should return nullopt (no complete line yet)
    auto line1 = socket.recvLine();
    EXPECT_FALSE(line1.has_value());

    // Server completes the line
    server->send("COME\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Now should get complete line
    auto line2 = socket.recvLine();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "WELCOME\n");
}

TEST_F(TcpSocketTest, RecvLineMultipleLinesInOnePacket)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // Server sends multiple lines at once
    server->send("WELCOME\nmsz 10 10\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // First recvLine should get first line
    auto line1 = socket.recvLine();
    ASSERT_TRUE(line1.has_value());
    EXPECT_EQ(*line1, "WELCOME\n");

    // Second recvLine should get second line (from buffer)
    auto line2 = socket.recvLine();
    ASSERT_TRUE(line2.has_value());
    EXPECT_EQ(*line2, "msz 10 10\n");
}

TEST_F(TcpSocketTest, PollReturnsTrue)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // Server sends data
    server->send("WELCOME\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Poll should return true
    EXPECT_TRUE(socket.poll(100));
}

TEST_F(TcpSocketTest, PollReturnsFalseOnTimeout)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // No data sent, poll should timeout
    EXPECT_FALSE(socket.poll(100));
}

TEST_F(TcpSocketTest, PollReturnsTrueWithBufferedLine)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // Server sends two lines
    server->send("WELCOME\nmsz 10 10\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Recv first line
    auto line1 = socket.recvLine();
    ASSERT_TRUE(line1.has_value());

    // Poll should return true immediately (buffered line available)
    EXPECT_TRUE(socket.poll(0));  // 0 timeout, should still return true
}

TEST_F(TcpSocketTest, RecvLineNoDataAvailable)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // No data sent, should return nullopt immediately (non-blocking)
    auto line = socket.recvLine();
    EXPECT_FALSE(line.has_value());
}

TEST_F(TcpSocketTest, ConnectionClosed)
{
    server->start();

    std::thread acceptThread([this]() { server->acceptConnection(); });

    TcpSocket socket;
    socket.connect("localhost", TEST_PORT);
    acceptThread.join();

    // Server closes connection
    server->stop();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // recvLine should return nullopt on closed connection
    auto line = socket.recvLine();
    EXPECT_FALSE(line.has_value());
}
