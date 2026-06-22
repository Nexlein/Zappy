#include "logging/LogObserver.hpp"

#include <string>

namespace {

    constexpr const char* NET = "NET";
    constexpr const char* WORLD = "WORLD";

    /// Strip a single trailing '\n' so protocol lines log on one row.
    std::string trim(const std::string& s)
    {
        if (!s.empty() && s.back() == '\n') return s.substr(0, s.size() - 1);
        return s;
    }

    std::string resStr(const Resources& r)
    {
        return "food=" + std::to_string(r.food) + " lin=" + std::to_string(r.linemate) +
               " der=" + std::to_string(r.deraumere) + " sib=" + std::to_string(r.sibur) +
               " men=" + std::to_string(r.mendiane) + " phi=" + std::to_string(r.phiras) +
               " thy=" + std::to_string(r.thystame);
    }

}  // namespace

LogObserver::LogObserver(Logger& logger) : _logger(logger) {}

void LogObserver::onClientConnected(int connectionId)
{
    _logger.info(NET, "client connected (conn=" + std::to_string(connectionId) + ")");
}

void LogObserver::onClientDisconnected(int connectionId)
{
    _logger.info(NET, "client disconnected (conn=" + std::to_string(connectionId) + ")");
}

void LogObserver::onLineReceived(int connectionId, const std::string& line)
{
    _logger.debug(NET, "<- conn=" + std::to_string(connectionId) + " : " + trim(line));
}

void LogObserver::onLineSent(int connectionId, const std::string& line)
{
    _logger.debug(NET, "-> conn=" + std::to_string(connectionId) + " : " + trim(line));
}

void LogObserver::onPlayerAdded(int playerId, int x, int y, Orientation, int level,
                                const std::string& teamName)
{
    _logger.info(WORLD, "player #" + std::to_string(playerId) + " added team=" + teamName +
                            " lvl=" + std::to_string(level) + " at (" + std::to_string(x) + "," +
                            std::to_string(y) + ")");
}

void LogObserver::onPlayerMoved(int playerId, int nx, int ny, Orientation)
{
    _logger.debug(WORLD, "player #" + std::to_string(playerId) + " moved to (" +
                             std::to_string(nx) + "," + std::to_string(ny) + ")");
}

void LogObserver::onPlayerInventoryChanged(int playerId, int x, int y, Resources inventory)
{
    _logger.debug(WORLD, "player #" + std::to_string(playerId) + " inventory at (" +
                             std::to_string(x) + "," + std::to_string(y) + ") [" +
                             resStr(inventory) + "]");
}

void LogObserver::onPlayerRemoved(int playerId)
{
    _logger.info(WORLD, "player #" + std::to_string(playerId) + " removed");
}

void LogObserver::onPlayerEjected(int playerId)
{
    _logger.info(WORLD, "player #" + std::to_string(playerId) + " ejected");
}

void LogObserver::onBroadcast(int playerId, const std::string& message)
{
    _logger.debug(WORLD, "player #" + std::to_string(playerId) + " broadcast: " + message);
}

void LogObserver::onResourceTaken(int playerId, ResourceType resourceType, int tileX, int tileY,
                                  Resources resources)
{
    _logger.debug(WORLD, "player #" + std::to_string(playerId) + " took " +
                             Resources::get_name(resourceType) + " at (" + std::to_string(tileX) +
                             "," + std::to_string(tileY) + ") tile now [" + resStr(resources) +
                             "]");
}

void LogObserver::onResourceDropped(int playerId, ResourceType resourceType, int tileX, int tileY,
                                    Resources resources)
{
    _logger.debug(WORLD, "player #" + std::to_string(playerId) + " dropped " +
                             Resources::get_name(resourceType) + " at (" + std::to_string(tileX) +
                             "," + std::to_string(tileY) + ") tile now [" + resStr(resources) +
                             "]");
}

void LogObserver::onEggLaid(int eggId, int playerId, int x, int y)
{
    _logger.info(WORLD, "egg #" + std::to_string(eggId) + " laid by #" + std::to_string(playerId) +
                            " at (" + std::to_string(x) + "," + std::to_string(y) + ")");
}

void LogObserver::onInitialEggSpawned(int eggId, const std::string& teamName, int x, int y)
{
    _logger.info(WORLD, "initial egg #" + std::to_string(eggId) + " for team " + teamName +
                            " at (" + std::to_string(x) + "," + std::to_string(y) + ")");
}

void LogObserver::onEggHatched(int eggId)
{
    _logger.info(WORLD, "egg #" + std::to_string(eggId) + " hatched");
}

void LogObserver::onEggDied(int eggId)
{
    _logger.info(WORLD, "egg #" + std::to_string(eggId) + " died");
}

void LogObserver::onIncantationStart(int x, int y, int level,
                                     const std::vector<int>& participantIds)
{
    _logger.info(WORLD, "incantation start lvl=" + std::to_string(level) + " at (" +
                            std::to_string(x) + "," + std::to_string(y) +
                            ") participants=" + std::to_string(participantIds.size()));
}

void LogObserver::onIncantationEnd(int x, int y, bool success)
{
    _logger.info(WORLD, std::string("incantation end ") + (success ? "success" : "fail") + " at (" +
                            std::to_string(x) + "," + std::to_string(y) + ")");
}

void LogObserver::onPlayerLevelUp(int playerId, int newLevel)
{
    _logger.info(WORLD,
                 "player #" + std::to_string(playerId) + " -> lvl " + std::to_string(newLevel));
}

void LogObserver::onGameEnd(const std::string& winningTeam)
{
    _logger.info(WORLD, "game end, winner=" + winningTeam);
}

void LogObserver::onTileChanged(int x, int y, Resources resources)
{
    _logger.debug(WORLD, "tile (" + std::to_string(x) + "," + std::to_string(y) + ") -> [" +
                             resStr(resources) + "]");
}
