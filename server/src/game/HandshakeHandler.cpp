#include "game/HandshakeHandler.hpp"

#include <algorithm>
#include <random>

#include "core/data/Orientation.hpp"
#include "protocol/Serializer.hpp"

HandshakeHandler::HandshakeHandler(ClientManager& clients, World& world, GuiNotifier& notifier,
                                   const ServerConfig& config, PromotedCallback onPromoted)
    : _clients(clients),
      _world(world),
      _notifier(notifier),
      _config(config),
      _onPromoted(onPromoted)
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
    if (_world.teamEggCount(line) == 0) {
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
    for (auto& [id, p] : _world.getPlayers())
        _notifier.send(connectionId,
                       Serializer::pnw(p.id, p.x, p.y, p.orientation, p.level, p.teamName));
    // Initial eggs (parentPlayerId < 0) replay as sse; player-laid eggs as enw,
    // matching how World::_spawnEgg first announced them.
    for (auto& [id, egg] : _world.getEggs()) {
        if (egg.parentPlayerId < 0)
            _notifier.send(connectionId, Serializer::sse(egg.id, egg.teamName, egg.x, egg.y));
        else
            _notifier.send(connectionId, Serializer::enw(egg.id, egg.parentPlayerId, egg.x, egg.y));
    }
}

void HandshakeHandler::_promoteToAi(int connectionId, const std::string& teamName)
{
    std::uniform_int_distribution<int> dori(1, 4);
    static std::mt19937 rng{std::random_device{}()};
    auto orientation = static_cast<Orientation>(dori(rng));

    // A player only spawns by hatching one of the team's eggs. popEggForTeam
    // fires onEggHatched through the observers (GUI ebo, logging).
    auto egg = _world.popEggForTeam(teamName);
    if (!egg) {
        _reject(connectionId);
        return;
    }

    int playerId = _world.addPlayer(connectionId, teamName, egg->x, egg->y, orientation);
    _clients.getConnection(connectionId).setType(ClientType::AI);
    _clients.getConnection(connectionId).setPlayerId(playerId);

    int slotsLeft = _world.teamEggCount(teamName);
    _clients.send(connectionId, std::to_string(slotsLeft) + "\n");
    _clients.send(connectionId,
                  std::to_string(_config.width) + " " + std::to_string(_config.height) + "\n");

    _onPromoted(connectionId, playerId);
}

void HandshakeHandler::_reject(int connectionId)
{
    _clients.send(connectionId, "ko\n");
    _clients.disconnect(connectionId);
}
