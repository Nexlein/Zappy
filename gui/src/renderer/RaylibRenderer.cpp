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
#include "raylib_helpers/SpaceSkybox.hpp"
#include "raylib_helpers/TextRenderer.hpp"
#include "raylib_helpers/TooltipRenderer.hpp"
#include "raylib_helpers/WinScreen.hpp"

void RaylibRenderer::init()
{
    _tileSlotMap.clear();
    _speedSlider.reset();
    _pendingSpeed.reset();
    _winScreen.reset();
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Zappy");
    SetTargetFPS(60);

    if (!TextRenderer::loadFont(std::string(FONT_PATH)))
        TraceLog(LOG_WARNING, "Failed to load font %s, using default", FONT_PATH.data());

    if (_savedWindow.valid) {
        SetWindowMonitor(_savedWindow.monitor);
        SetWindowSize(_savedWindow.width, _savedWindow.height);
        SetWindowPosition(static_cast<int>(_savedWindow.position.x),
                          static_cast<int>(_savedWindow.position.y));
        if (_savedWindow.fullscreen) ToggleFullscreen();
    }

    _cam.init(10, 10);

    _background = std::make_unique<SpaceSkybox>();
    _background->init();

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
        throw std::runtime_error("Failed to load crystal model: " +
                                 std::string(CRYSTAL_MODEL_PATH));

    _lightingShader =
        LoadShader("gui/assets/shaders/lighting.vs", "gui/assets/shaders/lighting.fs");
    _shaderViewPosLoc = GetShaderLocation(_lightingShader, "viewPos");

    for (int i = 0; i < _foodModel.materialCount; i++)
        _foodModel.materials[i].shader = _lightingShader;
    for (int i = 0; i < _crystalModel.materialCount; i++)
        _crystalModel.materials[i].shader = _lightingShader;

    _sun = CreateLight(LIGHT_DIRECTIONAL, {0.0f, 1000.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, WHITE,
                       _lightingShader);

    // Boost ambient so unlit faces aren't black
    float ambient[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    SetShaderValue(_lightingShader, GetShaderLocation(_lightingShader, "ambient"), ambient,
                   SHADER_UNIFORM_VEC4);
}

void RaylibRenderer::render()
{
    _initTeamColors();
    _updateSelection(GetFrameTime());
    _cam.update(GetFrameTime(), _state->world.width, _state->world.height, &_state->world);

    if (_background) {
        float scaledDelta = GetFrameTime();
        if (_state && _state->timeUnit > 0) scaledDelta *= (_state->timeUnit / 100.0f);
        _background->update(scaledDelta);
    }

    // Update shader camera position for specular lighting
    float camPos[3] = {_cam.camera().position.x, _cam.camera().position.y,
                       _cam.camera().position.z};
    SetShaderValue(_lightingShader, _shaderViewPosLoc, camPos, SHADER_UNIFORM_VEC3);

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode3D(_cam.camera());
    _render3D();

    EndMode3D();

    if (_state) {
        _hudWidget.setWorld(&_state->world);
        _hudWidget.setTimeUnit(_state->timeUnit);
        _hudWidget.setServerUptime(_state->serverUptimeSeconds);
        _hudWidget.setTeamColorFunc([this](const std::string& t) { return _getTeamColor(t); });
        _hudWidget.draw(_getScaledFontSize(18));
    }

    _render2D();
    EndDrawing();
}

void RaylibRenderer::handleInput()
{
    if (IsKeyPressed(KEY_F)) {
        _cam.isFreecamActive() ? _cam.exitFreecam() : _cam.enterFreecam();
    }

    _cam.handleInput();

    {
        _entityTooltip.setSelection(_selection);
        _entityTooltip.setWorld(_state ? &_state->world : nullptr);
        _entityTooltip.setTeamColorFunc([this](const std::string& t) { return _getTeamColor(t); });
        _entityTooltip.setFollowActive(_cam.isFollowActive());
        _entityTooltip.handleInput();
        if (int fid = _entityTooltip.popFollowRequest(); fid != -1) {
            if (fid == -2) {
                _cam.stopFollow();
            } else {
                _cam.startFollow(fid);
            }
        }

        _playerPanel.setWorld(_state ? &_state->world : nullptr);
        if (_playerPanel.handleInput()) {
            if (auto pSel = _playerPanel.getPendingSelection()) {
                _selection = *pSel;
            }
        }

        if (_speedSlider.handleInput()) {
            if (auto speed = _speedSlider.getPendingSpeedChange()) {
                _pendingSpeed = speed;
            }
        }

        int sh = GetScreenHeight();
        Rectangle panelRect = {10.0f, static_cast<float>(sh - SpeedSlider::PANEL_HEIGHT - 10),
                               static_cast<float>(SpeedSlider::PANEL_WIDTH),
                               static_cast<float>(SpeedSlider::PANEL_HEIGHT)};
        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !_cam.isFreecamActive() &&
            !_cam.isFollowActive() && !CheckCollisionPointRec(mouse, panelRect) &&
            !_entityTooltip.isFollowButtonHovered()) {
            if (!_playerPanel.isOpen()) _performRaycast();
        }

        if (_cam.isFollowActive())
            _selection = {SelectionFinder::EntityType::Player, _cam.followedPlayerId(), -1, -1};
    }

    if (IsKeyPressed(KEY_L)) {
        auto lang = I18n::getLanguage();
        I18n::setLanguage(lang == I18n::Language::EN ? I18n::Language::FR : I18n::Language::EN);
    }

    if (IsKeyPressed(KEY_F3)) {
        _devMode = !_devMode;
        _hudWidget.setDevMode(_devMode, _devPort, _devMachine);
    }
}

bool RaylibRenderer::shouldClose() { return WindowShouldClose() || _winScreen.quitRequested(); }

void RaylibRenderer::setDevMode(bool dev, int port, const std::string& machine)
{
    _devMode = dev;
    _devPort = port;
    _devMachine = machine;
    _hudWidget.setDevMode(dev, port, machine);
}

void RaylibRenderer::shutdown()
{
    if (_playerModel.meshCount > 0) UnloadModel(_playerModel);
    if (_eggModel.meshCount > 0) UnloadModel(_eggModel);
    if (_foodModel.meshCount > 0) UnloadModel(_foodModel);
    if (_crystalModel.meshCount > 0) UnloadModel(_crystalModel);
    if (_lightingShader.id > 0) UnloadShader(_lightingShader);
    TextRenderer::unloadFont();
    if (_background) _background->unload();

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
    if (_background) _background->draw(_cam.camera());
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
            for (int i = 0; i < 7; i++) rotations[i] = _tileSlotMap.resourceRotation(x, y, i);
            EntityRenderer::drawResources(
                res, slotIndices, rotations,
                RenderingHelper::tileToWorld(x, y, _state->world.width, _state->world.height,
                                             TILE_SIZE),
                TILE_SIZE, _foodModel, FOOD_MODEL_SIZE, _crystalModel, CRYSTAL_MODEL_SIZE,
                RESOURCE_SPHERE_BASE_SIZE);
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
        Vector2 screenPos = GetWorldToScreen(worldPos, _cam.camera());

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

    _entityTooltip.setSelection(_selection);
    _entityTooltip.setWorld(_state ? &_state->world : nullptr);
    _entityTooltip.setTeamColorFunc([this](const std::string& t) { return _getTeamColor(t); });
    _entityTooltip.setFollowActive(_cam.isFollowActive());
    _entityTooltip.draw(_getScaledFontSize(18));

    if (_state) {
        _playerPanel.setWorld(&_state->world);
        _playerPanel.setTeamColorFunc(
            [this](const std::string& teamName) { return _getTeamColor(teamName); });
        _playerPanel.draw(_getScaledFontSize(14));
    }

    _drawSpeedSlider();

    if (_state && !_state->winnerTeam.empty()) {
        _winScreen.setWinner(_state->winnerTeam, _getTeamColor(_state->winnerTeam));
        _winScreen.setDuration(_state->gameDurationSeconds, _state->gameDurationTicks);
        _winScreen.handleInput();
        _winScreen.draw(_getScaledFontSize(18));
    }
}

void RaylibRenderer::_drawSpeedSlider()
{
    if (_state) _speedSlider.syncFromServer(_state->timeUnit);
    _speedSlider.draw(_getScaledFontSize(18));
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

std::optional<int> RaylibRenderer::getPendingSpeedChange()
{
    auto val = _pendingSpeed;
    _pendingSpeed.reset();
    return val;
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

void RaylibRenderer::_performRaycast()
{
    if (!_state) return;

    Ray ray = GetMouseRay(GetMousePosition(), _cam.camera());
    _selection =
        SelectionFinder::findFromRay(ray, *_state, TILE_SIZE, _playerModel, PLAYER_MODEL_SIZE,
                                     _eggModel, EGG_MODEL_SIZE, _tileSlotMap);

    if (_selection.type == SelectionFinder::EntityType::None) {
        _selection = SelectionFinder::getEmptySelection();
    }
}

void RaylibRenderer::_updateSelection([[maybe_unused]] float deltaTime) {}

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