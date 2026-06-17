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

void GuiNotifier::onPlayerNew(const Player& p)
{
    broadcast(Serializer::pnw(p.id, p.x, p.y, p.orientation, p.level, p.teamName));
}

void GuiNotifier::onPlayerDeath(int id) { broadcast(Serializer::pdi(id)); }
