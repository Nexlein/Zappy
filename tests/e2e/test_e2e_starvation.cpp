/**
 * E2E starvation test — verifies that a player who runs out of food dies
 * correctly: the AI receives "dead\n" and the GUI receives "pdi #ID".
 *
 * Timing at freq=10000:
 *   - One food unit lasts 126000 ms / 10000 = 12.6 ms
 *   - Players start with 10 food → death after 10 × 12.6 ms = 126 ms
 *   - We wait 200 ms to be safely past the last tick, then nudge the server's
 *     poll loop and assert on the broadcasts.
 *
 * The server needs a poll() iteration to fire the scheduled starvation
 * callback. We nudge it by sending a harmless "sgt" on the GUI socket —
 * that unblocks the server's poll() and causes the scheduler to tick.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>

#include <gtest/gtest.h>

static constexpr int E2E_PORT = 14404;
static constexpr int CONNECT_RETRIES = 20;
static constexpr int CONNECT_RETRY_US = 50'000;

// Starvation at freq=10000: 126000/10000 = 12.6 ms per food unit, 10 units = 126 ms total.
// We wait 200 ms to be safely past the last tick.
static constexpr int STARVATION_WAIT_MS = 200;

// ---------------------------------------------------------------------------
// Wire helpers
// ---------------------------------------------------------------------------

static pid_t spawnServer()
{
    pid_t pid = fork();
    if (pid != 0) return pid;

    char* argv[] = {
        const_cast<char*>("./zappy_server"),
        const_cast<char*>("-p"), const_cast<char*>("14404"),
        const_cast<char*>("-x"), const_cast<char*>("10"),
        const_cast<char*>("-y"), const_cast<char*>("10"),
        const_cast<char*>("-n"), const_cast<char*>("TeamA"), const_cast<char*>("TeamB"),
        const_cast<char*>("-c"), const_cast<char*>("5"),
        const_cast<char*>("-f"), const_cast<char*>("10000"),
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

static std::string readLine(int fd, int timeoutSec = 2, int timeoutUsec = 0)
{
    std::string buf;
    char c;
    timeval tv{timeoutSec, timeoutUsec};
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

// GUI burst size: 1 msz + 100 bct + 2 tna + 1 sgt + 10 sse
static constexpr int GUI_BURST = 1 + 100 + 2 + 1 + 10;

static int connectAsGui()
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);                                        // WELCOME
    sendLine(fd, "GRAPHIC");
    for (int i = 0; i < GUI_BURST; ++i) readLine(fd);  // drain initial burst
    return fd;
}

static int connectAsAi(int& playerIdOut)
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);          // WELCOME
    sendLine(fd, "TeamA");
    readLine(fd);          // slots
    readLine(fd);          // map size
    playerIdOut = -1;
    return fd;
}

// Read lines from fd until one starts with prefix, up to maxLines attempts.
static std::string waitForLine(int fd, const std::string& prefix, int maxLines = 30)
{
    for (int i = 0; i < maxLines; ++i) {
        std::string line = readLine(fd);
        if (line.rfind(prefix, 0) == 0) return line;
    }
    return "";
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class E2EStarvation : public ::testing::Test {
    protected:
    pid_t serverPid = -1;
    void SetUp()    override { serverPid = spawnServer(); }
    void TearDown() override { if (serverPid > 0) stopServer(serverPid); }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// A player that never eats will exhaust 10 food units and die.
// The AI socket must receive "dead", and the GUI must receive "pdi #ID".
TEST_F(E2EStarvation, StarvedPlayerReceivesDeadAndGuiReceivesPdi)
{
    int guiFd = connectAsGui();
    ASSERT_GE(guiFd, 0) << "Could not connect GUI";

    int playerId = -1;
    int aiFd = connectAsAi(playerId);
    ASSERT_GE(aiFd, 0) << "Could not connect AI";

    // Learn the player's ID from the pnw broadcast the GUI receives on AI join.
    std::string pnwLine = waitForLine(guiFd, "pnw ");
    ASSERT_FALSE(pnwLine.empty()) << "GUI did not receive pnw after AI joined";
    std::string idStr;
    std::istringstream ss(pnwLine.substr(4));
    ss >> idStr;                              // "#N"
    playerId = std::stoi(idStr.substr(1));    // strip '#'

    // Wait for all 10 food units to be consumed at freq=10000.
    std::this_thread::sleep_for(std::chrono::milliseconds(STARVATION_WAIT_MS));

    // Nudge the server's poll() loop so the scheduler fires the starvation callback.
    sendLine(guiFd, "sgt");
    readLine(guiFd);   // consume sgt response

    // GUI must have received "pdi #ID" announcing the player's death.
    std::string pdiLine = waitForLine(guiFd, "pdi ");
    ASSERT_FALSE(pdiLine.empty()) << "GUI did not receive pdi after player starved";
    std::string expectedPdi = "pdi #" + std::to_string(playerId);
    EXPECT_EQ(pdiLine, expectedPdi);

    // The AI socket itself must have received "dead".
    std::string deadLine = waitForLine(aiFd, "dead", 5);
    EXPECT_EQ(deadLine, "dead");

    close(guiFd);
    close(aiFd);
}

// After a player dies, the GUI must NOT receive a second pdi for the same player.
// This guards against double-death bugs (e.g. starvation timer firing twice).
TEST_F(E2EStarvation, DeadPlayerDoesNotDieTwice)
{
    int guiFd = connectAsGui();
    ASSERT_GE(guiFd, 0);

    int playerId = -1;
    int aiFd = connectAsAi(playerId);
    ASSERT_GE(aiFd, 0);

    std::string pnwLine = waitForLine(guiFd, "pnw ");
    ASSERT_FALSE(pnwLine.empty());
    std::string idStr;
    std::istringstream ss(pnwLine.substr(4));
    ss >> idStr;
    playerId = std::stoi(idStr.substr(1));

    // Wait well past the first death, giving time for any erroneous second timer.
    std::this_thread::sleep_for(std::chrono::milliseconds(STARVATION_WAIT_MS * 2));

    // Nudge the server loop twice to flush any pending callbacks.
    sendLine(guiFd, "sgt");
    readLine(guiFd);
    sendLine(guiFd, "sgt");
    readLine(guiFd);

    // Collect all pdi lines that arrive within a short window (100 ms per read).
    std::string expectedPdi = "pdi #" + std::to_string(playerId);
    int pdiCount = 0;
    for (int i = 0; i < 10; ++i) {
        std::string line = readLine(guiFd, 0, 100'000);  // 100 ms timeout
        if (line == expectedPdi) ++pdiCount;
        if (line.empty()) break;  // timeout expired — nothing more queued
    }

    EXPECT_EQ(pdiCount, 1) << "Player died " << pdiCount << " times (expected exactly 1)";

    close(guiFd);
    close(aiFd);
}
