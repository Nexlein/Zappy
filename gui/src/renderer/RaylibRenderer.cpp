#include "RaylibRenderer.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "core/behaviors/ADrawableBehavior.hpp"
#include "raylib_helpers/ColorPalette.hpp"
#include "raylib_helpers/EntityRenderer.hpp"
#include "raylib_helpers/GridRenderer.hpp"
#include "raylib_helpers/RenderingHelper.hpp"
#include "raylib_helpers/TooltipRenderer.hpp"

void RaylibRenderer::init()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "Zappy");
    SetTargetFPS(60);

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
}

void RaylibRenderer::render()
{
    _initTeamColors();
    _updateSelection(GetFrameTime());
    _updateCamera(_state->world.width, _state->world.height);

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(_camera);
    _render3D();
    EndMode3D();

    _render2D();
    _drawHUD();
    EndDrawing();
}

void RaylibRenderer::handleInput()
{
    static double lastLeftClickTime = -1.0;
    // KEY_A maps to 'Q' on AZERTY
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) _cameraAngle += CAMERA_MOVE_SPEED * GetFrameTime();
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        _cameraAngle -= CAMERA_MOVE_SPEED * GetFrameTime();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        _performRaycast();
        if (GetTime() - lastLeftClickTime < SELECTION_DOUBLE_CLICK &&
            _selection.type != SelectionFinder::EntityType::None)
            _selection.permanent = true;
        lastLeftClickTime = GetTime();
    }
}

bool RaylibRenderer::shouldClose() { return WindowShouldClose(); }

void RaylibRenderer::shutdown()
{
    if (_playerModel.meshCount > 0) UnloadModel(_playerModel);

    CloseWindow();
}

void RaylibRenderer::_render3D()
{
    GridRenderer::drawGrid(_state->world.width, _state->world.height, TILE_SIZE);

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

    for (auto& [id, egg] : _state->world.eggs) {
        egg.visual.update(GetFrameTime());
        Vector3 worldPos = RenderingHelper::tileToWorld(egg.x, egg.y, _state->world.width,
                                                        _state->world.height, TILE_SIZE);
        EntityRenderer::drawEgg(worldPos, _getTeamColor(egg.team), _eggModel, egg.rotation,
                                _eggModelBaseMats, EGG_MODEL_SIZE * egg.visual.scale);
        _drawBehaviorParticles(egg.visual);
    }

    for (int x = 0; x < _state->world.width; x++) {
        for (int y = 0; y < _state->world.height; y++) {
            EntityRenderer::drawResources(
                _state->world.at(x, y), x, y,
                RenderingHelper::tileToWorld(x, y, _state->world.width, _state->world.height,
                                             TILE_SIZE),
                TILE_SIZE, RESOURCE_SPHERE_BASE_SIZE);
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

        Vector3 worldPos = group[0]->visual.pos;
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
            builder.addLine("Level " + std::to_string(p->level), _getTeamColor(p->team));

        builder.draw(screenPos);
    }

    _drawSelectedToolip();
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
                builder.addColoredText({"Tile", " is empty"}, {tileColor, textColor});
            } else {
                builder.addLine("Tile:", tileColor);
                _addResourceLines(builder, resources, "  ", textColor);
            }
            break;
        }

        case SelectionFinder::EntityType::Player: {
            if (!_state->world.playerExists(_selection.id)) return;
            const Player& player = _state->world.players.at(_selection.id);
            builder.addColoredText({"Player ", "#" + std::to_string(player.id)},
                                   {playerColor, textColor});
            builder.addColoredText({"  Team ", player.team},
                                   {textColor, _getTeamColor(player.team)});
            builder.addLine("  Level " + std::to_string(player.level), textColor);
            if (player.inventory.isEmpty()) {
                builder.addLine("  Inventory is empty", textColor);
            } else {
                builder.addLine("  Inventory:", textColor);
                _addResourceLines(builder, player.inventory, "    ", textColor);
            }
            break;
        }

        case SelectionFinder::EntityType::Egg: {
            if (!_state->world.eggExists(_selection.id)) return;
            const Egg& egg = _state->world.eggs.at(_selection.id);
            builder.addColoredText({"Egg ", "#" + std::to_string(egg.id)}, {eggColor, textColor});
            builder.addColoredText({"  Team ", egg.team}, {textColor, _getTeamColor(egg.team)});
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
    Color textColor = {150, 160, 180, 255};
    Color accentColor = {210, 220, 240, 255};

    int fps = GetFPS();
    Color fpsColor = fps >= 55 ? GREEN : (fps >= 30 ? YELLOW : RED);
    std::string fpsText = "FPS: " + std::to_string(fps);

    std::string mapText =
        "Map: " + std::to_string(_state->world.width) + "x" + std::to_string(_state->world.height);
    std::string timeUnitText = "Time unit: " + std::to_string(_state->timeUnit);

    int uptimeHours = _state->serverUptimeSeconds / 3600;
    int uptimeMinutes = (_state->serverUptimeSeconds % 3600) / 60;
    int uptimeSeconds = _state->serverUptimeSeconds % 60;
    std::string uptimeText = "Time ";
    if (uptimeHours > 0)
        uptimeText += std::to_string(uptimeHours) + "h ";
    if (uptimeMinutes > 0 || uptimeHours > 0)
        uptimeText += std::to_string(uptimeMinutes) + "m ";
    uptimeText += std::to_string(uptimeSeconds) + "s";

    std::unordered_map<std::string, int> teamPlayerCounts;
    for (const auto& teamName : _state->world.teams) teamPlayerCounts[teamName] = 0;
    for (const auto& [id, player] : _state->world.players) teamPlayerCounts[player.team]++;

    // Sort teams by player count (descending)
    std::vector<std::pair<std::string, int>> sortedTeams(teamPlayerCounts.begin(),
                                                         teamPlayerCounts.end());
    std::sort(sortedTeams.begin(), sortedTeams.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    auto builder = TooltipRenderer::create()
                       .addLine(fpsText, fpsColor)
                       .addLine(mapText, accentColor)
                       .addLine(timeUnitText, accentColor)
                       .addLine(uptimeText, textColor);

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
                                     _eggModel, EGG_MODEL_SIZE, SELECTION_TIMER);

    if (_selection.type == SelectionFinder::EntityType::None) {
        _selection = SelectionFinder::getEmptySelection();
    }
}

void RaylibRenderer::_updateSelection(float deltaTime)
{
    if (_selection.type == SelectionFinder::EntityType::None) return;
    if (_selection.permanent) return;

    _selection.timer -= deltaTime;
    if (_selection.timer <= 0.0f) {
        _selection = SelectionFinder::getEmptySelection();
    }
}

void RaylibRenderer::_addResourceLines(TooltipRenderer::Builder& builder, const Resources& res,
                                       const std::string& indent, Color color)
{
    for (int i = 0; i < 7; i++) {
        int qty = res[i];
        if (qty <= 0) continue;
        builder.addLine(indent + res.get_name(i) + ": " + std::to_string(qty), color);
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
            if (dx * dx + dz * dz < threshSq) {
                group.push_back(all[j]);
                assigned[j] = true;
            }
        }
        groups.push_back(std::move(group));
    }
    return groups;
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