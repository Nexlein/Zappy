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
#include "interfaces/IWorldObserver.hpp"

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
    /// @param seed value for the world RNG (egg + resource placement), for reproducible games.
    World(int width, int height, const std::vector<std::string>& teamNames, unsigned int seed);

    /// Access a tile by position. Map is toroidal, coordinates wrap.
    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    std::vector<std::pair<int, int>> spawnResources();

    int addPlayer(int connectionId, const std::string& teamName, int x, int y,
                  Orientation orientation);
    void removePlayer(int id);
    void movePlayer(int id, int x, int y);
    void turnPlayer(int id, Orientation orientation);

    bool takeResource(int playerId, ResourceType type);
    bool setResource(int playerId, ResourceType type);

    /**
     * @brief Consume one unit of food from @p playerId (starvation tick).
     * Fires onPlayerInventoryChanged so observers stay in sync.
     * @return true if the player is still alive (food > 0), false if it starved.
     */
    bool consumeFood(int playerId);

    Player& getPlayer(int id);

    /// Fire onBroadcast to observers (GUI animation, logging). No state change.
    void playerBroadcast(int playerId, const std::string& message);

    EjectResult ejectPlayers(int ejectorId);

    /// Spawn @p countPerTeam eggs for every team at random tiles (server startup).
    void spawnInitialEggs(int countPerTeam);

    int addEgg(int playerId);
    bool hatchEgg(int eggId);
    std::optional<Egg> popEggForTeam(const std::string& teamName);

    int teamEggCount(const std::string& team) const;

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
    bool finalizeIncantation(int x, int y, const std::vector<int>& participantIds);

    std::optional<std::string> checkWin() const;

    /// True once a team has won. Game logic stops reacting to AI commands.
    bool isGameEnded() const;
    /// Winning team name, set when isGameEnded() becomes true.
    const std::optional<std::string>& winner() const;

    int teamPlayerCount(const std::string& team) const;
    int width() const;
    int height() const;
    const std::unordered_map<int, Player>& getPlayers() const;
    const std::unordered_map<int, Egg>& getEggs() const;

    void addWorldObserver(IWorldObserver* observer);

    private:
    int _spawnEgg(const std::string& teamName, int x, int y, int parentPlayerId);

    int _width;
    int _height;
    std::vector<Tile> _tiles;
    std::unordered_map<int, Player> _players;
    std::unordered_map<int, Egg> _eggs;
    std::vector<std::string> _teamNames;
    std::mt19937 _rng;
    int _nextPlayerId = 0;
    int _nextEggId = 0;
    bool _gameEnded = false;
    std::optional<std::string> _winner;
    std::vector<IWorldObserver*> _observers;
};
