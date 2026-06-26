#include "SpaceSkybox.hpp"

#include <random>

#include "raymath.h"
#include "rlgl.h"

void SpaceSkybox::init()
{
    Mesh sphere = GenMeshSphere(500.0f, 32, 32);
    _sphereModel = LoadModelFromMesh(sphere);
    _generateTexture();
    _sphereModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = _spaceTexture;
}

void SpaceSkybox::_generateTexture()
{
    int width = 1024;
    int height = 512;

    // Generate a smooth gradient from deep dark blue -> black (0 degrees = vertical)
    Image img = GenImageGradientLinear(width, height, 0, {10, 5, 20, 255}, {0, 0, 0, 255});

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(0, width - 1);
    std::uniform_int_distribution<> distY(0, height - 1);
    std::uniform_int_distribution<> distIntensity(100, 255);

    // Add scattered stars
    for (int i = 0; i < 2000; ++i) {
        int x = distX(gen);
        int y = distY(gen);

        // Make most stars faint, some bright
        int intensity = distIntensity(gen);
        unsigned char c = static_cast<unsigned char>(intensity);

        ImageDrawPixel(&img, x, y, {c, c, static_cast<unsigned char>(c > 200 ? 255 : c), 255});
    }

    _spaceTexture = LoadTextureFromImage(img);
    UnloadImage(img);
}

void SpaceSkybox::update()
{
    // Slowly rotate the skybox to make the background feel alive
    _sphereModel.transform = MatrixMultiply(_sphereModel.transform, MatrixRotateY(0.0002f));
}

void SpaceSkybox::draw(const Camera3D& camera)
{
    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    // Draw centered perfectly around the camera so the player never reaches the edge
    DrawModel(_sphereModel, camera.position, 1.0f, WHITE);

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void SpaceSkybox::unload()
{
    UnloadTexture(_spaceTexture);
    UnloadModel(_sphereModel);
}
