#pragma once

#include <array>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "core/Resources.hpp"
#include "core/WorldState.hpp"

/**
 * @brief Manages 8-slot spatial assignment per tile for resources and eggs.
 *
 * Slots are shared between resource types and eggs on the same tile so they
 * never overlap each other or the player-reserved tile center.
 * Slot assignment is detected frame-by-frame from world state changes,
 * mirroring the pattern of the old _resourcePositions cache.
 */
class TileSlotMap {
    public:
    /** @brief Returns the XZ offset pair {dx, dz} for a slot index (0-7). */
    static std::pair<float, float> slotOffset(int slotIndex);

    /**
     * @brief Updates slot assignments for all resource types on a tile.
     * Assigns a slot when count goes 0→nonzero, releases when nonzero→0.
     * @return Slot index per resource type; -1 if that resource is absent.
     */
    std::array<int, 7> updateResourceSlots(int tileX, int tileY, const Resources& resources);

    /**
     * @brief Syncs egg slot assignments against the current egg map.
     * Assigns slots to new eggs, releases slots for eggs that have disappeared.
     */
    void syncEggs(const std::unordered_map<int, Egg>& eggs);

    /** @brief Returns the slot index for an egg id, or -1 if unknown. */
    int eggSlot(int eggId) const;

    /** @brief Returns the stable random Y rotation for a resource slot, or 0 if absent. */
    float resourceRotation(int tileX, int tileY, int resourceType) const;

    /** @brief Resets all state. Call on reconnect. */
    void clear();

    private:
    static constexpr int SLOT_COUNT = 8;
    static constexpr float CORNER = 0.35f;

    struct TileOccupancy {
        std::array<bool, 8> occupied = {};

        /** First free slot, set it occupied and return its index.
         *  If all 8 are taken, picks rand()%8 (overlap fallback, bit already set). */
        int acquire();

        /** Clears occupied[slotIndex]. */
        void release(int slotIndex);
    };

    std::unordered_map<uint64_t, TileOccupancy> _tileOccupancy;
    std::unordered_map<uint64_t, int> _resourceSlots;        // resourceKey → slot
    std::unordered_map<uint64_t, float> _resourceRotations;  // resourceKey → rotation
    std::unordered_map<int, int> _eggSlots;                  // eggId → slot
    std::unordered_map<int, std::pair<int, int>> _eggTile;   // eggId → {x, y}
    std::unordered_set<int> _knownEggs;

    static uint64_t tileKey(int x, int y);
    static uint64_t resourceKey(int x, int y, int resourceType);
};
