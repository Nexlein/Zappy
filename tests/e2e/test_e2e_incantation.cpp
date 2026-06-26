/**
 * E2E incantation test — verifies the full ritual flow over real TCP sockets.
 *
 * Strategy:
 *   1. Connect a GUI client and parse the initial bct burst to find a tile
 *      that already has a linemate stone on it (the server spawns them at start).
 *   2. Connect an AI client and query its spawn position via the GUI's ppo command.
 *   3. Navigate the AI to the linemate tile using Forward/Right/Left.
 *   4. Send Incantation.
 *   5. Assert the GUI receives "pic" (ritual started) then "pie ... 1" (success).
 *
 * Map is 3×3 so resources are dense and navigation is short.
 * freq=10000 so all timed delays (Forward=7 units, Incantation=300 units) are
 * effectively instant (~0.7ms and ~30ms respectively).
 */

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static constexpr int E2E_PORT = 14403;
static constexpr int MAP_W = 3;
static constexpr int MAP_H = 3;
static constexpr int TEAM_COUNT = 2;
static constexpr int CLIENTS_NB = 5;
// Burst: 1 msz + (w*h) bct + TEAM_COUNT tna + 1 sgt + (clientsNb*TEAM_COUNT) sse
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
                    const_cast<char*>("14403"),
                    const_cast<char*>("-x"),
                    const_cast<char*>("3"),
                    const_cast<char*>("-y"),
                    const_cast<char*>("3"),
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

// ---------------------------------------------------------------------------
// Protocol helpers
// ---------------------------------------------------------------------------

struct Tile {
    int x, y, linemate;
};
struct Pos {
    int x, y, orientation;
};  // orientation: 1=N 2=E 3=S 4=W

// Parse "bct X Y food linemate deraumere sibur mendiane phiras thystame"
static Tile parseBct(const std::string& line)
{
    // bct X Y f l d s m p t
    std::istringstream ss(line.substr(4));
    Tile t{};
    int food, d, s, m, p, th;
    ss >> t.x >> t.y >> food >> t.linemate >> d >> s >> m >> p >> th;
    return t;
}

// Connect as GUI, collect all bct lines from the burst, return fd.
// bctOut receives every parsed Tile.
static int connectAsGui(std::vector<Tile>& bctOut)
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);  // WELCOME
    sendLine(fd, "GRAPHIC");
    readLine(fd);  // msz line (first in burst)
    for (int i = 0; i < MAP_W * MAP_H; ++i) bctOut.push_back(parseBct(readLine(fd)));
    // drain remainder of burst (tna * TEAM_COUNT, sgt, sse * CLIENTS_NB * TEAM_COUNT)
    for (int i = 0; i < TEAM_COUNT + 1 + CLIENTS_NB * TEAM_COUNT; ++i) readLine(fd);
    return fd;
}

// Connect as AI, return fd. playerId is set to the player's server-assigned ID.
static int connectAsAi(int& playerId)
{
    int fd = connectWithRetry();
    if (fd < 0) return -1;
    readLine(fd);  // WELCOME
    sendLine(fd, "TeamA");
    readLine(fd);  // slots
    readLine(fd);  // map size

    // The server broadcasts pnw when a player connects.
    // We learn the player ID from it via the GUI — caller must do that.
    // For now just return the fd; playerId is filled by the caller via GUI.
    playerId = -1;
    return fd;
}

// Send one Forward and wait for "ok". At freq=10000 the delay is ~0.7ms so
// sleeping 20ms is more than enough.
static void stepForward(int aiFd)
{
    sendLine(aiFd, "Forward");
    readLine(aiFd);  // "ok"
}

// Turn right then forward.
static void turnRight(int aiFd)
{
    sendLine(aiFd, "Right");
    readLine(aiFd);  // "ok"
}

static void turnLeft(int aiFd)
{
    sendLine(aiFd, "Left");
    readLine(aiFd);  // "ok"
}

// Navigate from (fromX, fromY) facing `orientation` to (toX, toY) on a
// toroidal MAP_W × MAP_H grid. Uses cardinal moves only (no diagonal).
// Updates orientation in place.
static void navigateTo(int aiFd, int& x, int& y, int& ori, int toX, int toY)
{
    // ori: 1=N(y-1) 2=E(x+1) 3=S(y+1) 4=W(x-1)
    auto faceEast = [&]() {
        if (ori == 1) {
            turnRight(aiFd);
            ori = 2;
        } else if (ori == 3) {
            turnLeft(aiFd);
            ori = 2;
        } else if (ori == 4) {
            turnRight(aiFd);
            ori = 1;
            turnRight(aiFd);
            ori = 2;
        }
    };
    auto faceWest = [&]() {
        if (ori == 1) {
            turnLeft(aiFd);
            ori = 4;
        } else if (ori == 3) {
            turnRight(aiFd);
            ori = 4;
        } else if (ori == 2) {
            turnRight(aiFd);
            ori = 3;
            turnRight(aiFd);
            ori = 4;
        }
    };
    auto faceNorth = [&]() {
        if (ori == 2) {
            turnLeft(aiFd);
            ori = 1;
        } else if (ori == 4) {
            turnRight(aiFd);
            ori = 1;
        } else if (ori == 3) {
            turnRight(aiFd);
            ori = 4;
            turnRight(aiFd);
            ori = 1;
        }
    };
    auto faceSouth = [&]() {
        if (ori == 2) {
            turnRight(aiFd);
            ori = 3;
        } else if (ori == 4) {
            turnLeft(aiFd);
            ori = 3;
        } else if (ori == 1) {
            turnRight(aiFd);
            ori = 2;
            turnRight(aiFd);
            ori = 3;
        }
    };

    // Move along X axis first, then Y axis.
    while (x != toX) {
        int dx = ((toX - x) % MAP_W + MAP_W) % MAP_W;
        if (dx <= MAP_W / 2) {
            faceEast();
            stepForward(aiFd);
            x = (x + 1) % MAP_W;
        } else {
            faceWest();
            stepForward(aiFd);
            x = (x - 1 + MAP_W) % MAP_W;
        }
    }
    while (y != toY) {
        int dy = ((toY - y) % MAP_H + MAP_H) % MAP_H;
        if (dy <= MAP_H / 2) {
            faceSouth();
            stepForward(aiFd);
            y = (y + 1) % MAP_H;
        } else {
            faceNorth();
            stepForward(aiFd);
            y = (y - 1 + MAP_H) % MAP_H;
        }
    }
}

// Read lines from guiFd until a line starting with prefix is found (up to maxLines).
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

class E2EIncantation : public ::testing::Test {
    protected:
    pid_t serverPid = -1;

    void SetUp() override { serverPid = spawnServer(); }
    void TearDown() override
    {
        if (serverPid > 0) stopServer(serverPid);
    }
};

// ---------------------------------------------------------------------------
// Test
// ---------------------------------------------------------------------------

TEST_F(E2EIncantation, SinglePlayerLevelsUpToTwo)
{
    // 1. Connect GUI and collect tile resource info from the initial bct burst.
    std::vector<Tile> tiles;
    int guiFd = connectAsGui(tiles);
    ASSERT_GE(guiFd, 0) << "Could not connect GUI";

    // Find a tile that has at least 1 linemate (level 1→2 requirement).
    Tile* target = nullptr;
    for (auto& t : tiles) {
        if (t.linemate >= 1) {
            target = &t;
            break;
        }
    }
    ASSERT_NE(target, nullptr) << "No tile with linemate found in initial map state";

    // 2. Connect AI — server broadcasts pnw which GUI will receive.
    int playerId = -1;
    int aiFd = connectAsAi(playerId);
    ASSERT_GE(aiFd, 0) << "Could not connect AI";

    // 3. Read pnw from GUI to learn the player's spawn position and ID.
    std::string pnwLine = waitForLine(guiFd, "pnw ");
    ASSERT_FALSE(pnwLine.empty()) << "GUI did not receive pnw after AI joined";
    // pnw #ID X Y orientation level team
    std::istringstream pnwSS(pnwLine.substr(4));
    std::string idStr;
    int spawnX, spawnY, spawnOri, spawnLevel;
    std::string team;
    pnwSS >> idStr >> spawnX >> spawnY >> spawnOri >> spawnLevel >> team;
    playerId = std::stoi(idStr.substr(1));  // strip '#'

    // 4. Navigate to the linemate tile.
    int curX = spawnX, curY = spawnY, curOri = spawnOri;
    navigateTo(aiFd, curX, curY, curOri, target->x, target->y);

    // 5. Send Incantation. The server checks conditions and starts the ritual.
    sendLine(aiFd, "Incantation");

    // 6. GUI must receive "pic" announcing the ritual start.
    std::string picLine = waitForLine(guiFd, "pic ");
    ASSERT_FALSE(picLine.empty()) << "GUI did not receive pic broadcast";
    // pic X Y level #ID ...  — verify coordinates and level
    std::istringstream picSS(picLine.substr(4));
    int picX, picY, picLevel;
    picSS >> picX >> picY >> picLevel;
    EXPECT_EQ(picX, target->x);
    EXPECT_EQ(picY, target->y);
    EXPECT_EQ(picLevel, 1);

    // 7. Incantation delay = 300000 / freq=10000 = 30ms. Wait for it.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // The server needs a poll() iteration to fire the scheduled callback.
    // Sending any command triggers the server's poll loop to run. We use
    // a no-op GUI command so the AI player is not disturbed.
    sendLine(guiFd, "sgt");
    readLine(guiFd);  // consume sgt response

    // 8. GUI must receive "pie X Y 1" (success).
    std::string pieLine = waitForLine(guiFd, "pie ");
    ASSERT_FALSE(pieLine.empty()) << "GUI did not receive pie broadcast";
    EXPECT_NE(pieLine.find(" 1"), std::string::npos) << "pie result was not success (1)";

    // 9. Verify the AI client itself received "Current level: 2".
    std::string levelLine = waitForLine(aiFd, "Current level: ", 5);
    ASSERT_FALSE(levelLine.empty()) << "AI did not receive level-up message";
    EXPECT_EQ(levelLine, "Current level: 2");

    close(guiFd);
    close(aiFd);
}
