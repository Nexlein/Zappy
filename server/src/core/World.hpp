#pragma once

#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "data/Egg.hpp"
#include "data/Orientation.hpp"
#include "data/Player.hpp"
#include "data/Resources.hpp"
#include "data/Tile.hpp"

/// Result of an Eject command. dx/dy encode the push direction (used to notify ejected players).
struct EjectResult {
    std::vector<int> ejectedPlayerIds;
    int dx;
    int dy;
};

/**
 * @brief Stone and player requirements for one incantation level.
 * @see G-YEP-400_zappy.pdf for values per level.
 */
struct IncantationReq {
    int playerCount;
    int linemate;
    int deraumere;
    int sibur;
    int mendiane;
    int phiras;
    int thystame;
};

/**
 * @brief Pure game state - no networking, no scheduling.
 *
 * Owns the map, players, eggs, and teams. The server layer calls mutation
 * methods here after dispatching commands, then notifies GUI clients via
 * GuiNotifier. Keeping World network-free makes it fully unit-testable.
 *
 * @see server/doc.md - "core/World" section.
 * @see GuiNotifier for GUI notifications after mutations.
 */
class World {
    public:
    World(int width, int height, const std::vector<std::string>& teamNames);

    /// Access a tile by position. Map is toroidal, coordinates wrap.
    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    void spawnResources();

    int addPlayer(int connectionId, const std::string& teamName, int x, int y,
                  Orientation orientation);
    void removePlayer(int id);
    void movePlayer(int id, int x, int y);

    bool takeResource(int playerId, ResourceType type);
    bool setResource(int playerId, ResourceType type);

    Player& getPlayer(int id);

    EjectResult ejectPlayers(int ejectorId);

    int addEgg(int playerId);
    bool hatchEgg(int eggId);
    std::optional<Egg> popEggForTeam(const std::string& teamName);

    /**
     * @brief Validate and start an incantation for @p playerId.
     * Returns the list of participant IDs on success, nullopt if prerequisites fail.
     * Prerequisites are checked again in finalizeIncantation() after the ritual delay.
     */
    std::optional<std::vector<int>> startIncantation(int playerId);

    /**
     * @brief Finalize an incantation after the 300/f second delay.
     * Re-checks prerequisites. On success, levels up all participants and consumes stones.
     * Returns false if any participant died or moved off the tile.
     */
    bool finalizeIncantation(const std::vector<int>& participantIds);

    std::optional<std::string> checkWin() const;

    int teamPlayerCount(const std::string& team) const;
    int width() const;
    int height() const;
    const std::unordered_map<int, Player>& getPlayers() const;
    const std::unordered_map<int, Egg>& getEggs() const;

    private:
    int _width;
    int _height;
    std::vector<Tile> _tiles;
    std::unordered_map<int, Player> _players;
    std::unordered_map<int, Egg> _eggs;
    std::vector<std::string> _teamNames;
    std::mt19937 _rng{std::random_device{}()};
    int _nextPlayerId = 0;
    int _nextEggId = 0;
};
