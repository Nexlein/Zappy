#include "TileSlotMap.hpp"

#include <cstdlib>

static constexpr std::pair<float, float> SLOT_OFFSETS[8] = {
    {-0.35f, -0.35f},  // 0: corner
    { 0.35f, -0.35f},  // 1: corner
    {-0.35f,  0.35f},  // 2: corner
    { 0.35f,  0.35f},  // 3: corner
    { 0.00f, -0.35f},  // 4: edge
    { 0.00f,  0.35f},  // 5: edge
    {-0.35f,  0.00f},  // 6: edge
    { 0.35f,  0.00f},  // 7: edge
};

std::pair<float, float> TileSlotMap::slotOffset(int slotIndex)
{
    return SLOT_OFFSETS[slotIndex];
}

int TileSlotMap::TileOccupancy::acquire()
{
    for (int i = 0; i < 8; i++) {
        if (!occupied[i]) {
            occupied[i] = true;
            return i;
        }
    }
    // All slots taken: overlap fallback — pick random, bit already set
    return rand() % 8;
}

void TileSlotMap::TileOccupancy::release(int slotIndex)
{
    occupied[slotIndex] = false;
}

uint64_t TileSlotMap::tileKey(int x, int y)
{
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
           static_cast<uint32_t>(y);
}

uint64_t TileSlotMap::resourceKey(int x, int y, int resourceType)
{
    // pack x (20 bits), y (20 bits), type (4 bits) into 44 bits — safe for realistic map sizes
    return (static_cast<uint64_t>(x) << 24) |
           (static_cast<uint64_t>(y) <<  4) |
           static_cast<uint64_t>(resourceType);
}

std::array<int, 7> TileSlotMap::updateResourceSlots(int tileX, int tileY,
                                                     const Resources& resources)
{
    std::array<int, 7> result;
    result.fill(-1);

    for (int i = 0; i < 7; i++) {
        uint64_t rkey = resourceKey(tileX, tileY, i);
        bool hasSlot  = _resourceSlots.count(rkey) > 0;
        bool hasCount = resources[i] > 0;

        if (hasCount && !hasSlot) {
            int slot = _tileOccupancy[tileKey(tileX, tileY)].acquire();
            _resourceSlots[rkey] = slot;
            result[i] = slot;
        } else if (!hasCount && hasSlot) {
            _tileOccupancy[tileKey(tileX, tileY)].release(_resourceSlots[rkey]);
            _resourceSlots.erase(rkey);
        } else if (hasCount && hasSlot) {
            result[i] = _resourceSlots[rkey];
        }
    }

    return result;
}

void TileSlotMap::syncEggs(const std::unordered_map<int, Egg>& eggs)
{
    // Release slots for eggs that disappeared
    for (auto it = _knownEggs.begin(); it != _knownEggs.end();) {
        int id = *it;
        if (eggs.find(id) == eggs.end()) {
            auto tileIt = _eggTile.find(id);
            if (tileIt != _eggTile.end()) {
                auto [ex, ey] = tileIt->second;
                _tileOccupancy[tileKey(ex, ey)].release(_eggSlots[id]);
                _eggTile.erase(tileIt);
            }
            _eggSlots.erase(id);
            it = _knownEggs.erase(it);
        } else {
            ++it;
        }
    }

    // Assign slots for new eggs
    for (const auto& [id, egg] : eggs) {
        if (_knownEggs.find(id) == _knownEggs.end()) {
            int slot = _tileOccupancy[tileKey(egg.x, egg.y)].acquire();
            _eggSlots[id]  = slot;
            _eggTile[id]   = {egg.x, egg.y};
            _knownEggs.insert(id);
        }
    }
}

int TileSlotMap::eggSlot(int eggId) const
{
    auto it = _eggSlots.find(eggId);
    return it != _eggSlots.end() ? it->second : -1;
}

void TileSlotMap::clear()
{
    _tileOccupancy.clear();
    _resourceSlots.clear();
    _eggSlots.clear();
    _eggTile.clear();
    _knownEggs.clear();
}
