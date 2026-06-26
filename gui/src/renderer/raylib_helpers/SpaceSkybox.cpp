#include "SpaceSkybox.hpp"

#include <cmath>
#include <random>

#include "raymath.h"
#include "rlgl.h"

void SpaceSkybox::init()
{
    Mesh sphere = _generateCustomSphere(500.0f, 32, 32);
    _sphereModel = LoadModelFromMesh(sphere);
    _generateNightTexture();
    _generateDayTexture();
}

void SpaceSkybox::_generateNightTexture()
{
    int width = 1024;
    int height = 512;

    Image img = GenImageColor(width, height, BLANK);
    for (int y = 0; y < height; y++) {
        float t = static_cast<float>(y) / (height - 1);
        unsigned char r = static_cast<unsigned char>(10 * (1.0f - t));
        unsigned char g = static_cast<unsigned char>(5 * (1.0f - t));
        unsigned char b = static_cast<unsigned char>(20 * (1.0f - t));
        ImageDrawRectangle(&img, 0, y, width, 1, {r, g, b, 255});
    }

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
    SetTextureWrap(_nightTexture, TEXTURE_WRAP_CLAMP);
    UnloadImage(img);
}

void SpaceSkybox::_generateDayTexture()
{
    int width = 1024;
    int height = 512;

    Image img = GenImageColor(width, height, BLANK);
    for (int y = 0; y < height; y++) {
        float t = static_cast<float>(y) / (height - 1);
        unsigned char r = static_cast<unsigned char>(80 * (1.0f - t) + 180 * t);
        unsigned char g = static_cast<unsigned char>(150 * (1.0f - t) + 210 * t);
        unsigned char b = static_cast<unsigned char>(220 * (1.0f - t) + 240 * t);
        ImageDrawRectangle(&img, 0, y, width, 1, {r, g, b, 255});
    }

    _dayTexture = LoadTextureFromImage(img);
    SetTextureWrap(_dayTexture, TEXTURE_WRAP_CLAMP);
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

Mesh SpaceSkybox::_generateCustomSphere(float radius, int rings, int slices)
{
    Mesh mesh = {};
    int numVertices = (rings + 1) * (slices + 1);
    int numIndices = rings * slices * 6;

    mesh.vertexCount = numVertices;
    mesh.triangleCount = numIndices / 3;
    mesh.vertices = (float*)MemAlloc(numVertices * 3 * sizeof(float));
    mesh.texcoords = (float*)MemAlloc(numVertices * 2 * sizeof(float));
    mesh.normals = (float*)MemAlloc(numVertices * 3 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(numIndices * sizeof(unsigned short));

    int vIndex = 0;
    for (int i = 0; i <= rings; i++) {
        float v = static_cast<float>(i) / rings;
        float phi = v * PI;

        for (int j = 0; j <= slices; j++) {
            float u = static_cast<float>(j) / slices;
            float theta = u * PI * 2.0f;

            float x = std::cos(theta) * std::sin(phi);
            float y = std::cos(phi);
            float z = std::sin(theta) * std::sin(phi);

            mesh.vertices[vIndex * 3 + 0] = radius * x;
            mesh.vertices[vIndex * 3 + 1] = radius * y;
            mesh.vertices[vIndex * 3 + 2] = radius * z;

            mesh.normals[vIndex * 3 + 0] = x;
            mesh.normals[vIndex * 3 + 1] = y;
            mesh.normals[vIndex * 3 + 2] = z;

            mesh.texcoords[vIndex * 2 + 0] = u;
            mesh.texcoords[vIndex * 2 + 1] = v;
            vIndex++;
        }
    }

    int iIndex = 0;
    for (int i = 0; i < rings; i++) {
        for (int j = 0; j < slices; j++) {
            int current = i * (slices + 1) + j;
            int next = current + slices + 1;

            mesh.indices[iIndex++] = current;
            mesh.indices[iIndex++] = next;
            mesh.indices[iIndex++] = current + 1;

            mesh.indices[iIndex++] = current + 1;
            mesh.indices[iIndex++] = next;
            mesh.indices[iIndex++] = next + 1;
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
}
