/**
 * E2E handshake tests — spawn the real zappy_server binary and talk to it
 * over raw TCP sockets, exactly as a real client would.
 *
 * How it works:
 *   1. fork()  — duplicate the test process
 *   2. execv() — child replaces itself with ./zappy_server (real binary)
 *   3. parent  — connects a TCP socket, reads/writes raw bytes, asserts
 *   4. kill()  — parent kills the server at the end of each test
 *
 * Nothing server-internal is touched. The tests only see wire bytes.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <string>

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr int E2E_PORT = 14400;
static constexpr int CONNECT_RETRIES = 20;
static constexpr int CONNECT_RETRY_US = 50'000;  // 50 ms between retries

// Spawn ./zappy_server with a fixed config. Returns the child PID.
// The server listens on E2E_PORT, 10×10 map, teams A & B, 5 slots, freq 100.
static pid_t spawnServer()
{
    pid_t pid = fork();
    if (pid != 0) return pid;  // parent returns immediately with child PID

    // --- child: replace this process with the server binary ---
    char* argv[] = {
        const_cast<char*>("./zappy_server"),
        const_cast<char*>("-p"), const_cast<char*>("14400"),
        const_cast<char*>("-x"), const_cast<char*>("10"),
        const_cast<char*>("-y"), const_cast<char*>("10"),
        const_cast<char*>("-n"), const_cast<char*>("TeamA"), const_cast<char*>("TeamB"),
        const_cast<char*>("-c"), const_cast<char*>("5"),
        const_cast<char*>("-f"), const_cast<char*>("100"),
        nullptr
    };
    execv("./zappy_server", argv);
    // execv only returns on error
    _exit(1);
}

// Connect to 127.0.0.1:E2E_PORT, retrying until the server is ready.
// Returns a connected socket fd, or -1 on timeout.
static int connectWithRetry()
{
    for (int i = 0; i < CONNECT_RETRIES; ++i) {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(E2E_PORT);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
            return fd;  // success

        close(fd);
        usleep(CONNECT_RETRY_US);
    }
    return -1;
}

// Send a line to the server (appends \n).
static void sendLine(int fd, const std::string& line)
{
    std::string msg = line + "\n";
    write(fd, msg.c_str(), msg.size());
}

// Read from fd until a \n is found. Returns the line without the \n.
// Times out after ~2 seconds and returns whatever was read.
static std::string readLine(int fd)
{
    std::string buf;
    char c;
    // Set a read timeout so tests don't hang forever on a silent server
    timeval tv{2, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (read(fd, &c, 1) == 1) {
        if (c == '\n') break;
        buf += c;
    }
    return buf;
}

// Kill the server and reap the child to avoid zombies.
static void stopServer(pid_t pid)
{
    kill(pid, SIGTERM);
    waitpid(pid, nullptr, 0);
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class E2EHandshake : public ::testing::Test {
    protected:
    pid_t serverPid = -1;

    void SetUp() override { serverPid = spawnServer(); }

    void TearDown() override
    {
        if (serverPid > 0) stopServer(serverPid);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// Every client, regardless of type, must receive WELCOME as the first message.
TEST_F(E2EHandshake, NewClientReceivesWelcome)
{
    int fd = connectWithRetry();
    ASSERT_GE(fd, 0) << "Could not connect to server";

    std::string line = readLine(fd);
    EXPECT_EQ(line, "WELCOME");

    close(fd);
}

// Two independent clients must both receive WELCOME.
TEST_F(E2EHandshake, TwoClientsEachReceiveWelcome)
{
    int fd1 = connectWithRetry();
    int fd2 = connectWithRetry();
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);

    EXPECT_EQ(readLine(fd1), "WELCOME");
    EXPECT_EQ(readLine(fd2), "WELCOME");

    close(fd1);
    close(fd2);
}

// After sending a valid team name, the server responds with:
//   <remaining_slots>\n
//   <map_width> <map_height>\n
TEST_F(E2EHandshake, ValidTeamJoinReceivesSlotsAndMapSize)
{
    int fd = connectWithRetry();
    ASSERT_GE(fd, 0);

    readLine(fd);  // consume WELCOME
    sendLine(fd, "TeamA");

    std::string slots = readLine(fd);
    std::string mapSize = readLine(fd);

    // slots must be a non-negative integer
    EXPECT_FALSE(slots.empty());
    EXPECT_GE(std::stoi(slots), 0);

    // map size must match the -x 10 -y 10 we passed to the server
    EXPECT_EQ(mapSize, "10 10");

    close(fd);
}
