#include "SelectionFinder.hpp"

#include <cfloat>
#include <cmath>

SelectionFinder::Selection SelectionFinder::findFromRay(const Ray& ray, const GameState& state,
                                                        float tileSize, float playerHeight,
                                                        float eggHeight, float selectionDuration)
{
    float closestDist = FLT_MAX;
    Selection newSelection;

    // Check players
    for (const auto& [id, player] : state.world.players) {
        Vector3 pos =
            tileToWorld(player.x, player.y, state.world.width, state.world.height, tileSize);
        pos.y = playerHeight / 2.0f;  // Center of cube

        BoundingBox bbox = {
            {pos.x - playerHeight / 2.0f, pos.y - playerHeight / 2.0f, pos.z - playerHeight / 2.0f},
            {pos.x + playerHeight / 2.0f, pos.y + playerHeight / 2.0f, pos.z + playerHeight / 2.0f}};
        
        RayCollision collision = GetRayCollisionBox(ray, bbox);
        if (collision.hit && collision.distance < closestDist) {
            closestDist = collision.distance;
            newSelection.type = EntityType::Player;
            newSelection.id = id;
            newSelection.timer = selectionDuration;
        }
    }

    // Check eggs
    for (const auto& [id, egg] : state.world.eggs) {
        Vector3 pos = tileToWorld(egg.x, egg.y, state.world.width, state.world.height, tileSize);
        pos.y = eggHeight / 2.0f;  // Center of cube

        BoundingBox bbox = {
            {pos.x - eggHeight / 2.0f, pos.y - eggHeight / 2.0f, pos.z - eggHeight / 2.0f},
            {pos.x + eggHeight / 2.0f, pos.y + eggHeight / 2.0f, pos.z + eggHeight / 2.0f}};
        
        RayCollision collision = GetRayCollisionBox(ray, bbox);
        if (collision.hit && collision.distance < closestDist) {
            closestDist = collision.distance;
            newSelection.type = EntityType::Egg;
            newSelection.id = id;
            newSelection.timer = selectionDuration;
        }
    }

    // Check tiles (raycast to ground plane)
    RayCollision collision =
        GetRayCollisionQuad(ray, {-state.world.width / 2.0f, 0.0f, -state.world.height / 2.0f},
                            {state.world.width / 2.0f, 0.0f, -state.world.height / 2.0f},
                            {state.world.width / 2.0f, 0.0f, state.world.height / 2.0f},
                            {-state.world.width / 2.0f, 0.0f, state.world.height / 2.0f});

    if (collision.hit && collision.distance < closestDist) {
        float offsetX = (state.world.width * tileSize) / 2.0f;
        float offsetZ = (state.world.height * tileSize) / 2.0f;

        int tileX = (int)((collision.point.x + offsetX) / tileSize);
        int tileY = (int)((collision.point.z + offsetZ) / tileSize);

        if (tileX >= 0 && tileX < state.world.width && tileY >= 0 && tileY < state.world.height) {
            newSelection.type = EntityType::Tile;
            newSelection.tileX = tileX;
            newSelection.tileY = tileY;
            newSelection.timer = selectionDuration;
        }
    }

    return newSelection;
}

SelectionFinder::Selection SelectionFinder::getEmptySelection()
{
    return {
        .type = EntityType::None,
        .timer = 0.0f,
    };
}

Vector3 SelectionFinder::tileToWorld(int tileX, int tileY, int worldWidth, int worldHeight,
                                     float tileSize)
{
    float offsetX = (worldWidth * tileSize) / 2.0f;
    float offsetZ = (worldHeight * tileSize) / 2.0f;

    return {tileX * tileSize - offsetX + tileSize / 2.0f, 0.0f,
            tileY * tileSize - offsetZ + tileSize / 2.0f};
}

std::ostream& operator<<(std::ostream& os, const SelectionFinder::EntityType& type)
{
    switch (type) {
        case SelectionFinder::EntityType::None:
            return os << "None";
        case SelectionFinder::EntityType::Player:
            return os << "Player";
        case SelectionFinder::EntityType::Egg:
            return os << "Egg";
        case SelectionFinder::EntityType::Tile:
            return os << "Tile";
    }
    return os << "Unknown";
}

std::ostream& operator<<(std::ostream& os, const SelectionFinder::Selection& sel)
{
    os << "Selection{type=" << sel.type;
    if (sel.type == SelectionFinder::EntityType::Player ||
        sel.type == SelectionFinder::EntityType::Egg) {
        os << ", id=" << sel.id;
    } else if (sel.type == SelectionFinder::EntityType::Tile) {
        os << ", tile=(" << sel.tileX << "," << sel.tileY << ")";
    }
    os << ", timer=" << sel.timer << "}";
    return os;
}
