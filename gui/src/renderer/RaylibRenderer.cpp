#include "RaylibRenderer.hpp"

void RaylibRenderer::init()
{
    InitWindow(800, 600, "Zappy");
    SetTargetFPS(60);

    _camera = {
        .position = {0.0f, 10.0f, 10.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 45.0f,
        .projection = CAMERA_PERSPECTIVE
    };
}

void RaylibRenderer::render(const GameState& state)
{
    (void)state;

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(_camera);

    DrawGrid(10, 1.0f);
    DrawCube({0.0f, 0.5f, 0.0f}, 1.0f, 1.0f, 1.0f, RED);

    EndMode3D();

    DrawFPS(10, 10);

    EndDrawing();
}

bool RaylibRenderer::shouldClose()
{
    return WindowShouldClose();
}

void RaylibRenderer::shutdown()
{
    CloseWindow();
}
