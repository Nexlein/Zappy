#include "game/HandshakeHandler.hpp"

#include <algorithm>
#include <random>

#include "core/data/Orientation.hpp"
#include "protocol/Serializer.hpp"

HandshakeHandler::HandshakeHandler(ClientManager& clients, World& world, GuiNotifier& notifier,
                                   const ServerConfig& config)
    : _clients(clients), _world(world), _notifier(notifier), _config(config)
{
}

void HandshakeHandler::onNewConnection(int connectionId)
{
    _clients.send(connectionId, "WELCOME\n");
}

void HandshakeHandler::onLine(int connectionId, const std::string& line)
{
    if (line == "GRAPHIC") {
        _promoteToGui(connectionId);
        return;
    }
    auto it = std::find(_config.teamNames.begin(), _config.teamNames.end(), line);
    if (it == _config.teamNames.end()) {
        _reject(connectionId);
        return;
    }
    if (_world.teamPlayerCount(line) >= _config.clientsNb) {
        _reject(connectionId);
        return;
    }
    _promoteToAi(connectionId, line);
}

void HandshakeHandler::_promoteToGui(int connectionId)
{
    auto& conn = _clients.getConnection(connectionId);
    conn.setType(ClientType::GUI);
    _notifier.addGui(connectionId);

    _notifier.send(connectionId, Serializer::msz(_config.width, _config.height));
    for (int y = 0; y < _config.height; y++)
        for (int x = 0; x < _config.width; x++)
            _notifier.send(connectionId, Serializer::bct(x, y, _world.at(x, y).resources));
    for (auto& name : _config.teamNames) _notifier.send(connectionId, Serializer::tna(name));
    _notifier.send(connectionId, Serializer::sgt(_config.freq));
}

void HandshakeHandler::_promoteToAi(int connectionId, const std::string& teamName)
{
    std::uniform_int_distribution<int> dori(0, 3);
    static std::mt19937 rng{std::random_device{}()};
    auto orientation = static_cast<Orientation>(dori(rng));

    auto egg = _world.popEggForTeam(teamName);

    int x, y;
    if (egg) {
        x = egg->x;
        y = egg->y;
    } else {
        std::uniform_int_distribution<int> dx(0, _config.width - 1);
        std::uniform_int_distribution<int> dy(0, _config.height - 1);
        x = dx(rng);
        y = dy(rng);
    }

    int playerId = _world.addPlayer(connectionId, teamName, x, y, orientation);
    _clients.getConnection(connectionId).setType(ClientType::AI);
    _clients.getConnection(connectionId).setPlayerId(playerId);

    int slotsLeft = _config.clientsNb - _world.teamPlayerCount(teamName);
    _clients.send(connectionId, std::to_string(slotsLeft) + "\n");
    _clients.send(connectionId,
                  std::to_string(_config.width) + " " + std::to_string(_config.height) + "\n");

    if (egg) _notifier.broadcast(Serializer::ebo(egg->id));
    _notifier.onPlayerNew(_world.getPlayer(playerId));
}

void HandshakeHandler::_reject(int connectionId)
{
    _clients.send(connectionId, "ko\n");
    _clients.disconnect(connectionId);
}
