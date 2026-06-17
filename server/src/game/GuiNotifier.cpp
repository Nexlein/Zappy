#include "game/GuiNotifier.hpp"

#include <algorithm>

#include "protocol/Serializer.hpp"

GuiNotifier::GuiNotifier(ClientManager& clients) : _clients(clients) {}

void GuiNotifier::addGui(int connectionId) { _guiIds.push_back(connectionId); }

void GuiNotifier::removeGui(int connectionId)
{
    _guiIds.erase(std::remove(_guiIds.begin(), _guiIds.end(), connectionId), _guiIds.end());
}

void GuiNotifier::send(int connectionId, const std::string& msg)
{
    _clients.send(connectionId, msg);
}

void GuiNotifier::broadcast(const std::string& msg)
{
    for (int id : _guiIds) _clients.send(id, msg);
}

void GuiNotifier::onPlayerAdded(int playerId, int x, int y, Orientation orientation, int level,
                                const std::string& teamName)
{
    broadcast(Serializer::pnw(playerId, x, y, orientation, level, teamName));
}

void GuiNotifier::onPlayerMoved(int playerId, int nx, int ny, Orientation newOrientation)
{
    broadcast(Serializer::ppo(playerId, nx, ny, newOrientation));
}

void GuiNotifier::onPlayerRemoved(int playerId) { broadcast(Serializer::pdi(playerId)); }

void GuiNotifier::onPlayerEjected(int playerId) { broadcast(Serializer::pex(playerId)); }

void GuiNotifier::onBroadcast(int playerId, const std::string& message)
{
    broadcast(Serializer::pbc(playerId, message));
}

void GuiNotifier::onResourceTaken(int playerId, ResourceType resourceType, int tileX, int tileY,
                                  Resources resources)
{
    broadcast(Serializer::pgt(playerId, static_cast<int>(resourceType)));
    broadcast(Serializer::bct(tileX, tileY, resources));
}

void GuiNotifier::onResourceDropped(int playerId, ResourceType resourceType, int tileX, int tileY,
                                    Resources resources)
{
    broadcast(Serializer::pdr(playerId, static_cast<int>(resourceType)));
    broadcast(Serializer::bct(tileX, tileY, resources));
}

void GuiNotifier::onEggLaid(int eggId, int playerId, int x, int y)
{
    broadcast(Serializer::pfk(playerId));
    broadcast(Serializer::enw(eggId, playerId, x, y));
}

void GuiNotifier::onEggHatched(int eggId) { broadcast(Serializer::ebo(eggId)); }

void GuiNotifier::onIncantationStart(int x, int y, int level,
                                     const std::vector<int>& participantIds)
{
    broadcast(Serializer::pic(x, y, level, participantIds));
}

void GuiNotifier::onIncantationEnd(int x, int y, bool success)
{
    broadcast(Serializer::pie(x, y, success));
}

void GuiNotifier::onPlayerLevelUp(int playerId, int newLevel)
{
    broadcast(Serializer::plv(playerId, newLevel));
}

void GuiNotifier::onGameEnd(const std::string& winningTeam)
{
    broadcast(Serializer::seg(winningTeam));
}

void GuiNotifier::onTileChanged(int x, int y, Resources resources)
{
    broadcast(Serializer::bct(x, y, resources));
}