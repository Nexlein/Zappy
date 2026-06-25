#define RLIGHTS_IMPLEMENTATION
#include "RaylibRenderer.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "core/behaviors/ADrawableBehavior.hpp"
#include "raylib_helpers/ColorPalette.hpp"
#include "raylib_helpers/EntityRenderer.hpp"
#include "raylib_helpers/GridRenderer.hpp"
#include "raylib_helpers/I18n.hpp"
#include "raylib_helpers/RenderingHelper.hpp"
#include "raylib_helpers/TooltipRenderer.hpp"
#include "raymath.h"

void RaylibRenderer::init()
{
    _tileSlotMap.clear();
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Zappy");
    SetTargetFPS(60);

    if (_savedWindow.valid) {
        SetWindowMonitor(_savedWindow.monitor);
        SetWindowSize(_savedWindow.width, _savedWindow.height);
        SetWindowPosition(static_cast<int>(_savedWindow.position.x),
                          static_cast<int>(_savedWindow.position.y));
        if (_savedWindow.fullscreen) ToggleFullscreen();
    }

    _camera = {.position = {0.0f, 10.0f, 10.0f},
               .target = {0.0f, 0.0f, 0.0f},
               .up = {0.0f, 1.0f, 0.0f},
               .fovy = 45.0f,
               .projection = CAMERA_PERSPECTIVE};

    _selection = SelectionFinder::getEmptySelection();

    SetTraceLogLevel(LOG_ERROR);
    _playerModel = LoadModel(PLAYER_MODEL_PATH.data());
    SetTraceLogLevel(LOG_WARNING);
    if (_playerModel.meshCount == 0)
        throw std::runtime_error("Failed to load player model: " + std::string(PLAYER_MODEL_PATH));
    for (int i = 0; i < _playerModel.materialCount && i < 6; i++)
        _playerModelBaseMats[i] = _playerModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;

    SetTraceLogLevel(LOG_ERROR);
    _eggModel = LoadModel(EGG_MODEL_PATH.data());
    SetTraceLogLevel(LOG_WARNING);
    if (_eggModel.meshCount == 0)
        throw std::runtime_error("Failed to load egg model: " + std::string(EGG_MODEL_PATH));
    // set mat0 to a grayish white color
    _eggModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {235, 235, 235, 255};
    for (int i = 0; i < _eggModel.materialCount && i < 2; i++)
        _eggModelBaseMats[i] = _eggModel.materials[i].maps[MATERIAL_MAP_DIFFUSE].color;

    SetTraceLogLevel(LOG_ERROR);
    _foodModel = LoadModel(FOOD_MODEL_PATH.data());
    SetTraceLogLevel(LOG_WARNING);
    if (_foodModel.meshCount == 0)
        throw std::runtime_error("Failed to load food model: " + std::string(FOOD_MODEL_PATH));

    SetTraceLogLevel(LOG_ERROR);
    _crystalModel = LoadModel(CRYSTAL_MODEL_PATH.data());
    SetTraceLogLevel(LOG_WARNING);
    if (_crystalModel.meshCount == 0)
        throw std::runtime_error("Failed to load crystal model: " + std::string(CRYSTAL_MODEL_PATH));

    _lightingShader = LoadShader("gui/assets/shaders/lighting.vs", "gui/assets/shaders/lighting.fs");
    _shaderViewPosLoc = GetShaderLocation(_lightingShader, "viewPos");

    for (int i = 0; i < _foodModel.materialCount; i++)
        _foodModel.materials[i].shader = _lightingShader;
    for (int i = 0; i < _crystalModel.materialCount; i++)
        _crystalModel.materials[i].shader = _lightingShader;

    _sun = CreateLight(LIGHT_DIRECTIONAL, {0.0f, 1000.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, WHITE, _lightingShader);

    // Boost ambient so unlit faces aren't black
    float ambient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    SetShaderValue(_lightingShader, GetShaderLocation(_lightingShader, "ambient"), ambient, SHADER_UNIFORM_VEC4);
}

void RaylibRenderer::render()
{
    _initTeamColors();
    _updateSelection(GetFrameTime());
    _updateCamera(_state->world.width, _state->world.height);

    // Update shader camera position for specular lighting
    float camPos[3] = {_camera.position.x, _camera.position.y, _camera.position.z};
    SetShaderValue(_lightingShader, _shaderViewPosLoc, camPos, SHADER_UNIFORM_VEC3);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(_camera);
    _render3D();
    EndMode3D();

    _render2D();
    _drawHUD();
    EndDrawing();
}

void RaylibRenderer::_handleOrbitalInput()
{
    // KEY_A maps to 'Q' on AZERTY
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) _cameraAngle += CAMERA_MOVE_SPEED * GetFrameTime();
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        _cameraAngle -= CAMERA_MOVE_SPEED * GetFrameTime();
}

void RaylibRenderer::_enterFreecam()
{
    _savedOrbitalAngle = _cameraAngle;
    _savedOrbitalHeight = _cameraHeight;
    Vector3 dir = Vector3Subtract(_camera.target, _camera.position);
    _freecamYaw = atan2f(dir.z, dir.x);
    _freecamPitch = atan2f(dir.y, sqrtf(dir.x * dir.x + dir.z * dir.z));
    _freecamActive = true;
    DisableCursor();
}

void RaylibRenderer::_exitFreecam()
{
    _cameraAngle = _savedOrbitalAngle;
    _cameraHeight = _savedOrbitalHeight;
    _freecamActive = false;
    EnableCursor();
}

void RaylibRenderer::_handleFreecamInput()
{
    Vector2 delta = GetMouseDelta();
    _freecamYaw += delta.x * FREECAM_LOOK_SPEED;
    _freecamPitch -= delta.y * FREECAM_LOOK_SPEED;
    _freecamPitch = Clamp(_freecamPitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);

    Vector3 forward = {cosf(_freecamYaw) * cosf(_freecamPitch), sinf(_freecamPitch),
                       sinf(_freecamYaw) * cosf(_freecamPitch)};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0.0f, 1.0f, 0.0f}));
    float sp = FREECAM_MOVE_SPEED * GetFrameTime();

    // ZQSD on AZERTY = KEY_W/KEY_A/KEY_S/KEY_D in raylib
    if (IsKeyDown(KEY_W))
        _camera.position = Vector3Add(_camera.position, Vector3Scale(forward, sp));
    if (IsKeyDown(KEY_S))
        _camera.position = Vector3Add(_camera.position, Vector3Scale(forward, -sp));
    if (IsKeyDown(KEY_D)) _camera.position = Vector3Add(_camera.position, Vector3Scale(right, sp));
    if (IsKeyDown(KEY_A)) _camera.position = Vector3Add(_camera.position, Vector3Scale(right, -sp));
    if (IsKeyDown(KEY_SPACE)) _camera.position.y += sp;
    if (IsKeyDown(KEY_LEFT_SHIFT)) _camera.position.y -= sp;
}

void RaylibRenderer::handleInput()
{
    if (IsKeyPressed(KEY_F)) {
        _freecamActive ? _exitFreecam() : _enterFreecam();
    }

    if (_freecamActive)
        _handleFreecamInput();
    else
        _handleOrbitalInput();

    {
        _pendingSpeed = _speedSlider.handleInput();

        int sh = GetScreenHeight();
        Rectangle panelRect = {10.0f, static_cast<float>(sh - SpeedSlider::PANEL_HEIGHT - 10),
                               static_cast<float>(SpeedSlider::PANEL_WIDTH),
                               static_cast<float>(SpeedSlider::PANEL_HEIGHT)};
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !_freecamActive &&
            !CheckCollisionPointRec(mouse, panelRect))
            _performRaycast();
    }

    if (IsKeyPressed(KEY_L)) {
        auto lang = I18n::getLanguage();
        I18n::setLanguage(lang == I18n::Language::EN ? I18n::Language::FR : I18n::Language::EN);
    }

    if (IsKeyPressed(KEY_F3)) _devMode = !_devMode;
}

bool RaylibRenderer::shouldClose() { return WindowShouldClose(); }

void RaylibRenderer::setDevMode(bool dev, int port, const std::string& machine)
{
    _devMode = dev;
    _devPort = port;
    _devMachine = machine;
}

void RaylibRenderer::shutdown()
{
    if (_playerModel.meshCount > 0) UnloadModel(_playerModel);
    if (_eggModel.meshCount > 0) UnloadModel(_eggModel);
    if (_foodModel.meshCount > 0) UnloadModel(_foodModel);
    if (_crystalModel.meshCount > 0) UnloadModel(_crystalModel);
    if (_lightingShader.id > 0) UnloadShader(_lightingShader);

    _savedWindow = {
        .width = GetScreenWidth(),
        .height = GetScreenHeight(),
        .position = GetWindowPosition(),
        .monitor = GetCurrentMonitor(),
        .fullscreen = IsWindowFullscreen(),
        .valid = true,
    };

    CloseWindow();
}

void RaylibRenderer::_render3D()
{
    GridRenderer::drawTiles(_state->world.width, _state->world.height, TILE_SIZE);


    for (auto& [id, player] : _state->world.players) {
        player.visual.update(GetFrameTime());
        Vector3 worldPos = player.visual.pos;
        EntityRenderer::drawPlayer(worldPos, _getTeamColor(player.team), player.visual.angle,
                                   _playerModel, _playerModelBaseMats, PLAYER_MODEL_SIZE);
        _drawBehaviorParticles(player.visual);
    }

    for (auto& [id, player] : _state->world.dyingPlayers) {
        player.visual.update(GetFrameTime());
        Vector3 worldPos = player.visual.pos;
        EntityRenderer::drawPlayer(worldPos, _getTeamColor(player.team), player.visual.angle,
                                   _playerModel, _playerModelBaseMats,
                                   PLAYER_MODEL_SIZE * player.visual.scale);
        _drawBehaviorParticles(player.visual);
    }
    _state->world.purgeDyingPlayers();

    _tileSlotMap.syncEggs(_state->world.eggs);
    for (auto& [id, egg] : _state->world.eggs) {
        egg.visual.update(GetFrameTime());
        Vector3 worldPos = RenderingHelper::tileToWorld(egg.x, egg.y, _state->world.width,
                                                        _state->world.height, TILE_SIZE);
        int slot = _tileSlotMap.eggSlot(id);
        if (slot >= 0) {
            auto [dx, dz] = TileSlotMap::slotOffset(slot);
            worldPos.x += dx * TILE_SIZE;
            worldPos.z += dz * TILE_SIZE;
        }
        EntityRenderer::drawEgg(worldPos, _getTeamColor(egg.team), _eggModel, egg.rotation,
                                _eggModelBaseMats, EGG_MODEL_SIZE * egg.visual.scale);
        _drawBehaviorParticles(egg.visual);
    }

    for (int x = 0; x < _state->world.width; x++) {
        for (int y = 0; y < _state->world.height; y++) {
            const Resources& res = _state->world.at(x, y);
            auto slotIndices = _tileSlotMap.updateResourceSlots(x, y, res);
            std::array<float, 7> rotations;
            for (int i = 0; i < 7; i++)
                rotations[i] = _tileSlotMap.resourceRotation(x, y, i);
            EntityRenderer::drawResources(
                res, slotIndices, rotations,
                RenderingHelper::tileToWorld(x, y, _state->world.width, _state->world.height,
                                             TILE_SIZE),
                TILE_SIZE, _foodModel, FOOD_MODEL_SIZE, _crystalModel,
                CRYSTAL_MODEL_SIZE, RESOURCE_SPHERE_BASE_SIZE);
        }
    }

    _drawSelectionHighlight();
}

void RaylibRenderer::_drawBehaviorParticles(const VisualState& visual)
{
    for (const auto& b : visual.behaviors) {
        const auto* ab = dynamic_cast<const ADrawableBehavior*>(b.get());
        if (!ab) continue;
        for (const auto& line : ab->getLines()) {
            if (line.alpha <= 0.0f) continue;
            Color c = {line.color.r, line.color.g, line.color.b,
                       static_cast<unsigned char>(line.alpha * 255)};
            DrawLine3D(line.a, line.b, c);
        }
        for (const auto& p : ab->getParticles()) {
            if (!p.active) continue;
            Color c = {p.color.r, p.color.g, p.color.b, static_cast<unsigned char>(p.alpha * 255)};
            DrawSphere(p.pos, p.size, c);
        }
    }
}

void RaylibRenderer::_render2D()
{
    for (auto& group : _groupPlayersByVisualProximity()) {
        std::sort(group.begin(), group.end(),
                  [](const Player* a, const Player* b) { return a->level > b->level; });

        Vector3 worldPos = _groupLabelAnchor(group);
        worldPos.y = PLAYER_MODEL_SIZE * 2.0f;
        Vector2 screenPos = GetWorldToScreen(worldPos, _camera);

        auto builder = TooltipRenderer::create()
                           .setAnchor(TooltipRenderer::Anchor::BottomCenter)
                           .setBackgroundColor({180, 180, 180, 255})
                           .setBackgroundAlpha(160)
                           .setBorderColor(BLACK)
                           .setBorderThickness(1)
                           .setPadding(4)
                           .setFontSize(_getScaledFontSize(12));

        for (const Player* p : group)
            builder.addLine(
                std::string(I18n::get(I18n::Key::PLAYER_HEAD_LEVEL)) + std::to_string(p->level),
                _getTeamColor(p->team));

        builder.draw(screenPos);
    }

    _drawSelectedToolip();
    _drawSpeedSlider();
}

void RaylibRenderer::_drawSelectionHighlight()
{
    if (_selection.type == SelectionFinder::EntityType::None) return;

    switch (_selection.type) {
        case SelectionFinder::EntityType::Tile:
            GridRenderer::drawTileHighlight(_selection.tileX, _selection.tileY, _state->world.width,
                                            _state->world.height, TILE_SIZE, SELECTION_COLOR,
                                            SELECTION_LINE_THICKNESS);
            break;

        case SelectionFinder::EntityType::Player:
            if (_state->world.players.find(_selection.id) != _state->world.players.end()) {
                const Player& player = _state->world.players.at(_selection.id);
                BoundingBox bbox = GetModelBoundingBox(_playerModel);
                float topY = bbox.max.y * PLAYER_MODEL_SIZE;
                _drawSelectionArrow(player.visual.pos, topY);
            }
            break;

        case SelectionFinder::EntityType::Egg:
            if (_state->world.eggs.find(_selection.id) != _state->world.eggs.end()) {
                const Egg& egg = _state->world.eggs.at(_selection.id);
                Vector3 eggPos = RenderingHelper::tileToWorld(egg.x, egg.y, _state->world.width,
                                                              _state->world.height, TILE_SIZE);
                int slot = _tileSlotMap.eggSlot(_selection.id);
                if (slot >= 0) {
                    auto [dx, dz] = TileSlotMap::slotOffset(slot);
                    eggPos.x += dx * TILE_SIZE;
                    eggPos.z += dz * TILE_SIZE;
                }
                BoundingBox bbox = GetModelBoundingBox(_eggModel);
                float topY = bbox.max.y * EGG_MODEL_SIZE * egg.visual.scale;
                _drawSelectionArrow(eggPos, topY);
            }
            break;
        default:
            return;
    }
}

void RaylibRenderer::_drawSelectedToolip()
{
    if (_selection.type == SelectionFinder::EntityType::None) return;

    Color bgColor = {20, 25, 35, 220};
    Color borderColor = {60, 70, 90, 200};
    Color textColor = {255, 255, 255, 255};
    Color tileColor = {100, 200, 255, 255};
    Color playerColor = {100, 255, 150, 255};
    Color eggColor = {255, 200, 80, 255};

    auto builder = TooltipRenderer::create()
                       .setAnchor(TooltipRenderer::Anchor::TopRight)
                       .setBackgroundColor(bgColor)
                       .setBackgroundAlpha(180)
                       .setBorderColor(borderColor)
                       .setBorderThickness(2)
                       .setPadding(10)
                       .setFontSize(_getScaledFontSize(18));

    switch (_selection.type) {
        case SelectionFinder::EntityType::Tile: {
            const Resources& resources = _state->world.at(_selection.tileX, _selection.tileY);
            if (resources.isEmpty()) {
                builder.addColoredText(
                    {I18n::get(I18n::Key::LABEL_TILE), I18n::get(I18n::Key::TILE_EMPTY)},
                    {tileColor, textColor});
            } else {
                builder.addLine(I18n::get(I18n::Key::TILE_HEADER), tileColor);
                _addResourceLines(builder, resources, "  ", textColor);
            }
            break;
        }

        case SelectionFinder::EntityType::Player: {
            if (!_state->world.playerExists(_selection.id)) return;
            const Player& player = _state->world.players.at(_selection.id);
            builder.addColoredText(
                {I18n::get(I18n::Key::LABEL_PLAYER), " #" + std::to_string(player.id)},
                {playerColor, textColor});
            builder.addColoredText({I18n::get(I18n::Key::PLAYER_TEAM), player.team},
                                   {textColor, _getTeamColor(player.team)});
            builder.addLine(
                std::string(I18n::get(I18n::Key::PLAYER_LEVEL)) + std::to_string(player.level),
                textColor);
            if (player.inventory.isEmpty()) {
                builder.addLine(I18n::get(I18n::Key::PLAYER_INVENTORY_EMPTY), textColor);
            } else {
                builder.addLine(I18n::get(I18n::Key::PLAYER_INVENTORY), textColor);
                _addResourceLines(builder, player.inventory, "    ", textColor);
            }
            break;
        }

        case SelectionFinder::EntityType::Egg: {
            if (!_state->world.eggExists(_selection.id)) return;
            const Egg& egg = _state->world.eggs.at(_selection.id);
            builder.addColoredText({I18n::get(I18n::Key::LABEL_EGG), " #" + std::to_string(egg.id)},
                                   {eggColor, textColor});
            builder.addColoredText({I18n::get(I18n::Key::EGG_TEAM), egg.team},
                                   {textColor, _getTeamColor(egg.team)});
            break;
        }

        default:
            return;
    }

    builder.draw({GetScreenWidth() - 10.0f, 10.0f});
}

void RaylibRenderer::_drawHUD()
{
    Color bgColor = {20, 25, 35, 220};
    Color borderColor = {60, 70, 90, 200};
    Color accentColor = {210, 220, 240, 255};

    std::string mapText = std::string(I18n::get(I18n::Key::HUD_MAP)) +
                          std::to_string(_state->world.width) + "x" +
                          std::to_string(_state->world.height);

    std::string uptimeText;
    if (_state->serverUptimeSeconds == 0) {
        uptimeText = I18n::get(I18n::Key::HUD_UPTIME_UNKNOWN);
    } else {
        int uptimeHours = _state->serverUptimeSeconds / 3600;
        int uptimeMinutes = (_state->serverUptimeSeconds % 3600) / 60;
        int uptimeSeconds = _state->serverUptimeSeconds % 60;
        uptimeText = I18n::get(I18n::Key::HUD_UPTIME_PREFIX);
        if (uptimeHours > 0)
            uptimeText += std::to_string(uptimeHours) + I18n::get(I18n::Key::HUD_UPTIME_H);
        if (uptimeMinutes > 0 || uptimeHours > 0)
            uptimeText += std::to_string(uptimeMinutes) + I18n::get(I18n::Key::HUD_UPTIME_M);
        uptimeText += std::to_string(uptimeSeconds) + I18n::get(I18n::Key::HUD_UPTIME_S);
    }

    std::unordered_map<std::string, int> teamPlayerCounts;
    for (const auto& teamName : _state->world.teams) teamPlayerCounts[teamName] = 0;
    for (const auto& [id, player] : _state->world.players) teamPlayerCounts[player.team]++;

    // Sort teams by player count (descending)
    std::vector<std::pair<std::string, int>> sortedTeams(teamPlayerCounts.begin(),
                                                         teamPlayerCounts.end());
    std::sort(sortedTeams.begin(), sortedTeams.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    auto builder = TooltipRenderer::create();

    if (_devMode) {
        int fps = GetFPS();
        Color fpsColor = fps >= 55 ? GREEN : (fps >= 30 ? YELLOW : RED);
        builder.addLine(std::string(I18n::get(I18n::Key::HUD_FPS)) + std::to_string(fps), fpsColor);
        builder.addLine(
            std::string(I18n::get(I18n::Key::HUD_TIME_UNIT)) + std::to_string(_state->timeUnit),
            accentColor);
        builder.addLine(std::string(I18n::get(I18n::Key::HUD_PORT)) + std::to_string(_devPort),
                        accentColor);
        builder.addLine(std::string(I18n::get(I18n::Key::HUD_MACHINE)) + _devMachine, accentColor);
    }

    builder.addLine(mapText, accentColor).addLine(uptimeText, accentColor);

    // Add top 5 teams by population
    for (size_t i = 0; i < std::min(sortedTeams.size(), size_t(5)); i++) {
        const auto& [teamName, playerCount] = sortedTeams[i];
        std::string teamLine = teamName + ": " + std::to_string(playerCount);
        builder.addLine(teamLine, _getTeamColor(teamName));
    }

    builder.setBackgroundColor(bgColor)
        .setBackgroundAlpha(180)
        .setBorderColor(borderColor)
        .setBorderThickness(2)
        .setPadding(10)
        .setFontSize(_getScaledFontSize(18))
        .setAnchor(TooltipRenderer::Anchor::TopLeft)
        .draw({10.0f, 10.0f});
}

std::optional<int> RaylibRenderer::getPendingSpeedChange()
{
    auto val = _pendingSpeed;
    _pendingSpeed.reset();
    return val;
}

void RaylibRenderer::_drawSpeedSlider()
{
    _speedSlider.syncFromServer(_state->timeUnit);
    _speedSlider.draw(_getScaledFontSize(18));
}

void RaylibRenderer::_initTeamColors()
{
    if (_teamColors.size() == _state->world.teams.size()) return;
    _teamColors.clear();
    for (const auto& teamName : _state->world.teams)
        _teamColors[teamName] = ColorPalette::getTeamColor(_teamColors.size());
}

Color RaylibRenderer::_getTeamColor(const std::string& teamName)
{
    auto it = _teamColors.find(teamName);
    if (it != _teamColors.end()) return it->second;
    // fallback for teams not in tna list (shouldn't happen)
    return WHITE;
}

int RaylibRenderer::_getScaledFontSize(int baseFontSize) const
{
    // Scale based on height: 600px = 1.0x, 1200px = 2.0x
    // Clamp between 0.5x and 2.5x

    int screenHeight = GetScreenHeight();
    float scale = screenHeight / 600.0f;
    scale = std::max(0.5f, std::min(scale, 2.5f));
    return static_cast<int>(baseFontSize * scale);
}

void RaylibRenderer::_updateCamera(float worldWidth, float worldHeight)
{
    if (_freecamActive) {
        Vector3 dir = {cosf(_freecamYaw) * cosf(_freecamPitch), sinf(_freecamPitch),
                       sinf(_freecamYaw) * cosf(_freecamPitch)};
        _camera.target = Vector3Add(_camera.position, dir);
        return;
    }

    float maxDim = std::max(worldWidth, worldHeight);
    float adaptiveRadius = maxDim * 1.25f;

    float rawAspectX = worldWidth / maxDim;
    float rawAspectZ = worldHeight / maxDim;

    // Lerp toward circle (50% blend)
    float aspectX = 0.5f * (1.0f + rawAspectX);
    float aspectZ = 0.5f * (1.0f + rawAspectZ);

    _camera.position.x = cos(_cameraAngle) * adaptiveRadius * aspectX;
    _camera.position.z = sin(_cameraAngle) * adaptiveRadius * aspectZ;

    float currentRadius = sqrt(pow(_camera.position.x, 2) + pow(_camera.position.z, 2));

    float heightScale = currentRadius / adaptiveRadius;
    _camera.position.y = _cameraHeight * heightScale;

    _camera.target = {0.0f, 0.0f, 0.0f};
}

void RaylibRenderer::_performRaycast()
{
    if (!_state) return;

    Ray ray = GetMouseRay(GetMousePosition(), _camera);
    _selection =
        SelectionFinder::findFromRay(ray, *_state, TILE_SIZE, _playerModel, PLAYER_MODEL_SIZE,
                                     _eggModel, EGG_MODEL_SIZE, _tileSlotMap);

    if (_selection.type == SelectionFinder::EntityType::None) {
        _selection = SelectionFinder::getEmptySelection();
    }
}

void RaylibRenderer::_updateSelection([[maybe_unused]] float deltaTime) {}

void RaylibRenderer::_addResourceLines(TooltipRenderer::Builder& builder, const Resources& res,
                                       const std::string& indent, Color color)
{
    for (int i = 0; i < 7; i++) {
        int qty = res[i];
        if (qty <= 0) continue;
        builder.addLine(indent + I18n::resourceName(i) + ": " + std::to_string(qty), color);
    }
}

std::vector<std::vector<const Player*>> RaylibRenderer::_groupPlayersByVisualProximity() const
{
    constexpr float thresh = TILE_SIZE / 4.0f;
    constexpr float threshSq = thresh * thresh;

    std::vector<const Player*> all;
    all.reserve(_state->world.players.size());
    for (const auto& [id, player] : _state->world.players) all.push_back(&player);

    std::vector<bool> assigned(all.size(), false);
    std::vector<std::vector<const Player*>> groups;

    for (size_t i = 0; i < all.size(); i++) {
        if (assigned[i]) continue;
        std::vector<const Player*> group = {all[i]};
        assigned[i] = true;
        const Vector3& pi = all[i]->visual.pos;
        for (size_t j = i + 1; j < all.size(); j++) {
            if (assigned[j]) continue;
            const Vector3& pj = all[j]->visual.pos;
            float dx = pi.x - pj.x;
            float dz = pi.z - pj.z;
            bool nearbyVisual = dx * dx + dz * dz < threshSq;
            bool sameIncant = all[i]->incanting && all[j]->incanting && all[i]->x == all[j]->x &&
                              all[i]->y == all[j]->y;
            if (nearbyVisual || sameIncant) {
                group.push_back(all[j]);
                assigned[j] = true;
            }
        }
        groups.push_back(std::move(group));
    }
    return groups;
}

Vector3 RaylibRenderer::_groupLabelAnchor(const std::vector<const Player*>& group) const
{
    if (group.size() > 1) {
        const Player* ref = group[0];
        bool allIncantingOnSameTile = ref->incanting;
        for (size_t k = 1; allIncantingOnSameTile && k < group.size(); k++)
            allIncantingOnSameTile =
                group[k]->incanting && group[k]->x == ref->x && group[k]->y == ref->y;
        if (allIncantingOnSameTile)
            return RenderingHelper::tileToWorld(ref->x, ref->y, _state->world.width,
                                                _state->world.height, TILE_SIZE);
    }
    return group[0]->visual.pos;
}

void RaylibRenderer::_drawSelectionArrow(Vector3 basePos, float modelTopY) const
{
    // bob up and down using a sine wave
    float bob = sinf(static_cast<float>(GetTime()) * 4.0f) * 0.08f;

    float arrowBase = basePos.y + modelTopY + 0.05f + bob;
    float shaftHeight = 0.18f;
    float shaftRadius = 0.03f;
    float headHeight = 0.14f;
    float headRadius = 0.08f;

    // shaft sits above the arrowhead
    Vector3 shaftBot = {basePos.x, arrowBase + headHeight, basePos.z};
    DrawCylinder(shaftBot, shaftRadius, shaftRadius, shaftHeight, 8, SELECTION_COLOR);

    // cone: startPos at bottom, wide base there, tip (radius=0) at top → points down toward entity
    Vector3 coneBottom = {basePos.x, arrowBase, basePos.z};
    DrawCylinder(coneBottom, headRadius, 0.0f, headHeight, 8, SELECTION_COLOR);
}