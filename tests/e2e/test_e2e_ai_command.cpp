/**
 * E2E AI command tests — verify that a fully-handshaked AI client can send
 * game commands and receive correct protocol responses from the real server.
 *
 * The server runs at HIGH_FREQ (10000) so all timed delays (~7ms per unit)
 * become negligible and responses arrive almost immediately.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

#include <gtest/gtest.h>

static constexpr int E2E_PORT = 14401;
static constexpr int CONNECT_RETRIES = 20;
static constexpr int CONNECT_RETRY_US = 50'000;

// ---------------------------------------------------------------------------
// Wire helpers (same pattern as test_e2e_handshake.cpp)
// ---------------------------------------------------------------------------

static pid_t spawnServer()
{
    pid_t pid = fork();
    if (pid != 0) return pid;

    char* argv[] = {
        const_cast<char*>("./zappy_server"),
        const_cast<char*>("-p"), const_cast<char*>("14401"),
        const_cast<char*>("-x"), const_cast<char*>("10"),
        const_cast<char*>("-y"), const_cast<char*>("10"),
        const_cast<char*>("-n"), const_cast<char*>("TeamA"), const_cast<char*>("TeamB"),
        const_cast<char*>("-c"), const_cast<char*>("5"),
        const_cast<char*>("-f"), const_cast<char*>("10000"),  // HIGH_FREQ: delays ~= 0
        nullptr
    };
    execv("./zappy_server", argv);
    _exit(1);
}

static int connectWithRetry()
{
    for (int i = 0; i < CONNECT_RETRIES; ++i) {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(E2E_PORT);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) return fd;
        close(fd);
        usleep(CONNECT_RETRY_US);
    }
    return -1;
}

static void sendLine(int fd, const std::string& line)
{
    std::string msg = line + "\n";
    write(fd, msg.c_str(), msg.size());
}

static std::string readLine(int fd)
{
    std::string buf;
    char c;
    timeval tv{2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    while (read(fd, &c, 1) == 1) {
        if (c == '\n') break;
        buf += c;
    }
    return buf;
}

static void stopServer(pid_t pid)
{
    kill(pid, SIGTERM);
    waitpid(pid, nullptr, 0);
}

// ---------------------------------------------------------------------------
// Helper: complete the full AI handshake and return a ready-to-play socket fd.
//   WELCOME  ← server sends automatically on connect
//   TeamA\n  → client sends team name
//   <slots>  ← server replies with remaining egg count
//   <w> <h>  ← server replies with map dimensions
// After this, the connection is a live AI player.
// ---------------------------------------------------------------------------

static int connectAsAi()
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);           // consume "WELCOME"
    sendLine(fd, "TeamA");
    readLine(fd);           // consume slots count
    readLine(fd);           // consume map size
    return fd;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class E2EAiCommand : public ::testing::Test {
    protected:
    pid_t serverPid = -1;

    void SetUp() override { serverPid = spawnServer(); }
    void TearDown() override { if (serverPid > 0) stopServer(serverPid); }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Forward is a timed command: the server schedules it and sends "ok\n" once done.
// At freq=10000 the delay is 7000/10000 = 0.7 ms — negligible on any machine.
TEST_F(E2EAiCommand, ForwardRespondsWithOk)
{
    int fd = connectAsAi();
    ASSERT_GE(fd, 0) << "Could not connect and handshake as AI";

    sendLine(fd, "Forward");

    std::string response = readLine(fd);
    EXPECT_EQ(response, "ok");

    close(fd);
}

// Inventory is also timed. Response format: [ food N, linemate N, ... ]
TEST_F(E2EAiCommand, InventoryRespondsWithBracketLine)
{
    int fd = connectAsAi();
    ASSERT_GE(fd, 0);

    sendLine(fd, "Inventory");

    std::string response = readLine(fd);
    // Must start with '[' and end with ']'
    ASSERT_FALSE(response.empty());
    EXPECT_EQ(response.front(), '[');
    EXPECT_EQ(response.back(), ']');

    close(fd);
}

// An unknown command must get an immediate "ko" response (no scheduling).
TEST_F(E2EAiCommand, UnknownCommandRespondsWithKo)
{
    int fd = connectAsAi();
    ASSERT_GE(fd, 0);

    sendLine(fd, "garbage");

    std::string response = readLine(fd);
    EXPECT_EQ(response, "ko");

    close(fd);
}
