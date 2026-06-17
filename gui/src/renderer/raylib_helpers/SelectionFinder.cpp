#include "SelectionFinder.hpp"

#include <cfloat>
#include <cmath>

#include "RenderingHelper.hpp"
#include "raymath.h"

SelectionFinder::Selection SelectionFinder::findFromRay(const Ray& ray, const GameState& state,
                                                        float tileSize, const Model& playerModel,
                                                        float playerModelSize,
                                                        const Model& eggModel, float eggModelSize,
                                                        float selectionDuration)
{
    float closestDist = FLT_MAX;
    Selection newSelection;

    // Check players using mesh-accurate raycasting
    for (const auto& [id, player] : state.world.players) {
        Vector3 pos = player.visual.pos;
        Matrix transform =
            MatrixMultiply(MatrixScale(playerModelSize, playerModelSize, playerModelSize),
                           MatrixTranslate(pos.x, pos.y, pos.z));

        for (int m = 0; m < playerModel.meshCount; m++) {
            RayCollision collision = GetRayCollisionMesh(ray, playerModel.meshes[m], transform);
            if (collision.hit && collision.distance < closestDist) {
                closestDist = collision.distance;
                newSelection.type = EntityType::Player;
                newSelection.id = id;
                newSelection.timer = selectionDuration;
            }
        }
    }

    // Check eggs using mesh-accurate raycasting
    for (const auto& [id, egg] : state.world.eggs) {
        Vector3 pos = RenderingHelper::tileToWorld(egg.x, egg.y, state.world.width,
                                                   state.world.height, tileSize);
        Matrix transform = MatrixMultiply(MatrixScale(eggModelSize, eggModelSize, eggModelSize),
                                          MatrixTranslate(pos.x, pos.y, pos.z));

        for (int m = 0; m < eggModel.meshCount; m++) {
            RayCollision collision = GetRayCollisionMesh(ray, eggModel.meshes[m], transform);
            if (collision.hit && collision.distance < closestDist) {
                closestDist = collision.distance;
                newSelection.type = EntityType::Egg;
                newSelection.id = id;
                newSelection.timer = selectionDuration;
            }
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
    return {.type = EntityType::None,
            .id = -1,
            .tileX = -1,
            .tileY = -1,
            .timer = 0.0f,
            .permanent = false};
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
    os << ", timer=" << sel.timer << ", permanent=" << sel.permanent << "}";
    return os;
}
