/**
 * E2E slot exhaustion tests — verifies that the server correctly rejects
 * connections when a team has no eggs left.
 *
 * Config: clientsNb=5, so TeamA starts with 5 eggs.
 * After 5 AI clients join TeamA, the 6th must receive "ko" and be disconnected.
 */

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>
#include <vector>

static constexpr int E2E_PORT = 14405;
static constexpr int CLIENTS_NB = 5;
static constexpr int CONNECT_RETRIES = 20;
static constexpr int CONNECT_RETRY_US = 50'000;

// ---------------------------------------------------------------------------
// Wire helpers
// ---------------------------------------------------------------------------

static pid_t spawnServer()
{
    pid_t pid = fork();
    if (pid != 0) return pid;

    char* argv[] = {const_cast<char*>("./zappy_server"),
                    const_cast<char*>("-p"),
                    const_cast<char*>("14405"),
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

// Join as AI. Returns fd on success (slots + map size consumed).
// On rejection the server sends "ko" and closes — fd is still returned so
// the caller can read the rejection line itself.
static int joinTeam(const std::string& team, std::string& firstResponse)
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);  // WELCOME
    sendLine(fd, team);
    firstResponse = readLine(fd);
    return fd;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class E2ESlotExhaustion : public ::testing::Test {
    protected:
    pid_t serverPid = -1;
    std::vector<int> openFds;

    void SetUp() override { serverPid = spawnServer(); }

    void TearDown() override
    {
        for (int fd : openFds) close(fd);
        if (serverPid > 0) stopServer(serverPid);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

// After CLIENTS_NB clients fill all TeamA eggs, the next one gets "ko".
TEST_F(E2ESlotExhaustion, SixthClientOnFullTeamReceivesKo)
{
    // Fill all 5 TeamA slots.
    for (int i = 0; i < CLIENTS_NB; ++i) {
        std::string resp;
        int fd = joinTeam("TeamA", resp);
        ASSERT_GE(fd, 0) << "Could not connect client " << i;
        // Each successful join replies with the remaining slot count (an integer).
        EXPECT_FALSE(resp.empty());
        EXPECT_NE(resp, "ko") << "Client " << i << " was rejected unexpectedly";
        readLine(fd);  // consume map size line
        openFds.push_back(fd);
    }

    // 6th attempt — no eggs left for TeamA.
    std::string resp;
    int fd = joinTeam("TeamA", resp);
    ASSERT_GE(fd, 0) << "Could not connect 6th client";
    EXPECT_EQ(resp, "ko");
    close(fd);
}

// Slot count reported to each client must decrease as players join.
TEST_F(E2ESlotExhaustion, SlotCountDecreasesWithEachJoin)
{
    int prevSlots = CLIENTS_NB;  // first client sees clientsNb-1 remaining after itself

    for (int i = 0; i < CLIENTS_NB; ++i) {
        std::string resp;
        int fd = joinTeam("TeamA", resp);
        ASSERT_GE(fd, 0);
        ASSERT_NE(resp, "ko") << "Unexpected rejection at slot " << i;

        int slots = std::stoi(resp);
        EXPECT_EQ(slots, prevSlots - 1) << "Unexpected slot count at join " << i;
        prevSlots = slots;

        readLine(fd);  // consume map size
        openFds.push_back(fd);
    }
}

// Exhausting TeamA must not affect TeamB — a TeamB client must still be accepted.
TEST_F(E2ESlotExhaustion, OtherTeamUnaffectedByExhaustion)
{
    // Fill TeamA.
    for (int i = 0; i < CLIENTS_NB; ++i) {
        std::string resp;
        int fd = joinTeam("TeamA", resp);
        ASSERT_GE(fd, 0);
        readLine(fd);  // map size
        openFds.push_back(fd);
    }

    // TeamB must still accept a connection.
    std::string resp;
    int fd = joinTeam("TeamB", resp);
    ASSERT_GE(fd, 0);
    EXPECT_NE(resp, "ko") << "TeamB was incorrectly rejected after TeamA exhaustion";
    readLine(fd);  // map size
    close(fd);
}

// Sending an unknown team name must immediately return "ko".
TEST_F(E2ESlotExhaustion, UnknownTeamNameReceivesKo)
{
    std::string resp;
    int fd = joinTeam("NotATeam", resp);
    ASSERT_GE(fd, 0);
    EXPECT_EQ(resp, "ko");
    close(fd);
}
