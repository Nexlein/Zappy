/**
 * E2E GUI tests — verify the GUI protocol over real TCP sockets.
 *
 * GUI handshake sequence (after WELCOME):
 *   client → "GRAPHIC\n"
 *   server ← "msz <w> <h>\n"          (first line, always)
 *   server ← "bct ..." × (w*h)        (one per tile)
 *   server ← "tna ..." × (team count)
 *   server ← "sgt <freq>\n"
 *   server ← "sse ..." / "enw ..."    (one per existing egg)
 *
 * Only after draining this initial burst is the GUI in "live" mode and able
 * to send commands or observe real-time broadcasts.
 */

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

static constexpr int E2E_PORT = 14402;
static constexpr int CONNECT_RETRIES = 20;
static constexpr int CONNECT_RETRY_US = 50'000;

// Map and team config must match what we pass to the server binary.
static constexpr int MAP_W = 10;
static constexpr int MAP_H = 10;
static constexpr int TEAM_COUNT = 2;  // TeamA + TeamB
// Initial burst size: 1 msz + (w*h) bct + TEAM_COUNT tna + 1 sgt + (clientsNb*TEAM_COUNT) sse
// clientsNb=5, so 5*2=10 eggs → 10 sse lines.  Total = 1+100+2+1+10 = 114 lines.
static constexpr int GUI_BURST_LINES = 1 + (MAP_W * MAP_H) + TEAM_COUNT + 1 + (5 * TEAM_COUNT);

// ---------------------------------------------------------------------------
// Wire helpers
// ---------------------------------------------------------------------------

static pid_t spawnServer()
{
    pid_t pid = fork();
    if (pid != 0) return pid;

    char* argv[] = {const_cast<char*>("./zappy_server"),
                    const_cast<char*>("-p"),
                    const_cast<char*>("14402"),
                    const_cast<char*>("-x"),
                    const_cast<char*>("10"),
                    const_cast<char*>("-y"),
                    const_cast<char*>("10"),
                    const_cast<char*>("-n"),
                    const_cast<char*>("TeamA"),
                    const_cast<char*>("TeamB"),
                    const_cast<char*>("-c"),
                    const_cast<char*>("5"),
                    const_cast<char*>("-f"),
                    const_cast<char*>("10000"),
                    nullptr};
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

// Complete the GUI handshake and return the connected fd.
// After this call, the socket is in live mode: initial burst fully consumed.
static int connectAsGui()
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);  // consume "WELCOME"
    sendLine(fd, "GRAPHIC");
    for (int i = 0; i < GUI_BURST_LINES; ++i) readLine(fd);  // drain initial burst
    return fd;
}

// Complete the AI handshake and return the connected fd.
static int connectAsAi()
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);  // consume "WELCOME"
    sendLine(fd, "TeamA");
    readLine(fd);  // consume slots
    readLine(fd);  // consume map size
    return fd;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class E2EGui : public ::testing::Test {
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

// The very first line sent after "GRAPHIC" must be "msz <w> <h>".
TEST_F(E2EGui, FirstResponseIsMsz)
{
    int fd = connectWithRetry();
    ASSERT_GE(fd, 0);

    readLine(fd);  // consume "WELCOME"
    sendLine(fd, "GRAPHIC");

    std::string line = readLine(fd);
    EXPECT_EQ(line, "msz 10 10");

    close(fd);
}

// After the initial burst, sending "msz" must get an immediate "msz <w> <h>" back.
TEST_F(E2EGui, MszCommandReturnsMapSize)
{
    int fd = connectAsGui();
    ASSERT_GE(fd, 0) << "Could not connect and handshake as GUI";

    sendLine(fd, "msz");

    std::string response = readLine(fd);
    EXPECT_EQ(response, "msz 10 10");

    close(fd);
}

// When an AI client connects, the GUI must receive a "pnw" broadcast
// announcing the new player.
TEST_F(E2EGui, PnwBroadcastOnAiConnect)
{
    int guiFd = connectAsGui();
    ASSERT_GE(guiFd, 0) << "Could not connect GUI";

    // Now connect an AI — the server will broadcast pnw to all GUI clients.
    int aiFd = connectAsAi();
    ASSERT_GE(aiFd, 0) << "Could not connect AI";

    // Read up to 5 lines from the GUI socket — pnw must be among them.
    // (There may also be ebo for the hatched egg arriving first.)
    bool foundPnw = false;
    for (int i = 0; i < 5 && !foundPnw; ++i) {
        std::string line = readLine(guiFd);
        if (line.rfind("pnw ", 0) == 0) foundPnw = true;
    }

    EXPECT_TRUE(foundPnw) << "GUI did not receive a pnw broadcast after AI joined";

    close(guiFd);
    close(aiFd);
}
