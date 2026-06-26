#include "SpaceSkybox.hpp"

#include <cmath>
#include <random>

#include "raymath.h"
#include "rlgl.h"

void SpaceSkybox::init()
{
    Mesh sphere = GenMeshSphere(500.0f, 32, 32);
    _sphereModel = LoadModelFromMesh(sphere);
    _generateNightTexture();
    _generateDayTexture();
}

void SpaceSkybox::_generateNightTexture()
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

        int intensity = distIntensity(gen);
        unsigned char c = static_cast<unsigned char>(intensity);

        ImageDrawPixel(&img, x, y, {c, c, static_cast<unsigned char>(c > 200 ? 255 : c), 255});
    }

    _nightTexture = LoadTextureFromImage(img);
    UnloadImage(img);
}

void SpaceSkybox::_generateDayTexture()
{
    int width = 1024;
    int height = 512;

    // Generate a smooth daytime gradient (deep sky blue to light horizon)
    Image img = GenImageGradientLinear(width, height, 0, {80, 150, 220, 255}, {180, 210, 240, 255});

    _dayTexture = LoadTextureFromImage(img);
    UnloadImage(img);
}

void SpaceSkybox::update(float deltaTime)
{
    // Advance time. Full cycle every 20 seconds of scaled time
    _timeOfDay += deltaTime * 0.05f;
    if (_timeOfDay > 1.0f) _timeOfDay -= 1.0f;

    // Slowly rotate the skybox
    _sphereModel.transform = MatrixMultiply(_sphereModel.transform, MatrixRotateY(0.0002f));
}

void SpaceSkybox::draw(const Camera3D& camera)
{
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    rlEnableColorBlend();

    _sphereModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = _nightTexture;
    DrawModel(_sphereModel, camera.position, 1.0f, WHITE);

    // Day intensity (smooth sine wave 0.0 to 1.0)
    float dayFactor = std::sin(_timeOfDay * 2.0f * PI) * 0.5f + 0.5f;

    if (dayFactor > 0.01f) {
        unsigned char alpha = static_cast<unsigned char>(dayFactor * 255.0f);
        _sphereModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = _dayTexture;
        DrawModel(_sphereModel, camera.position, 1.0f, {255, 255, 255, alpha});
    }

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void SpaceSkybox::unload()
{
    UnloadTexture(_nightTexture);
    UnloadTexture(_dayTexture);
    UnloadModel(_sphereModel);
}
