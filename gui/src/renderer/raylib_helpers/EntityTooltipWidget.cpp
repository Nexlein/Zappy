/// @file EntityTooltipWidget.cpp

#include "EntityTooltipWidget.hpp"

#include <algorithm>

#include "I18n.hpp"
#include "TextRenderer.hpp"

static constexpr Color BG_COLOR = {20, 25, 35, 220};
static constexpr Color BORDER_COLOR = {60, 70, 90, 200};
static constexpr Color TEXT_COLOR = {255, 255, 255, 255};
static constexpr Color TILE_COLOR = {100, 200, 255, 255};
static constexpr Color PLAYER_COLOR = {100, 255, 150, 255};
static constexpr Color EGG_COLOR = {255, 200, 80, 255};
static constexpr Color FOLLOW_NORMAL = {40, 80, 60, 220};
static constexpr Color FOLLOW_HOVER = {60, 140, 90, 240};
static constexpr Color STOP_NORMAL = {80, 40, 40, 220};
static constexpr Color STOP_HOVER = {140, 60, 60, 240};

EntityTooltipWidget::EntityTooltipWidget()
{
    _tooltip.setAnchor(TooltipWidget::Anchor::TopRight)
        .setBackgroundColor(BG_COLOR)
        .setBackgroundAlpha(180)
        .setBorderColor(BORDER_COLOR)
        .setBorderThickness(2)
        .setPadding(10);

    _followBtn.setSize(BTN_W, BTN_H)
        .setRoundness(0.3f)
        .setBorderColor(BORDER_COLOR)
        .setBorderThickness(2.0f)
        .setTextColor(TEXT_COLOR);
}

void EntityTooltipWidget::setSelection(const SelectionFinder::Selection& sel) { _selection = sel; }
void EntityTooltipWidget::setWorld(const WorldState* world) { _world = world; }
void EntityTooltipWidget::setTeamColorFunc(std::function<Color(const std::string&)> func)
{
    _colorFunc = std::move(func);
}
void EntityTooltipWidget::setFollowActive(bool active) { _followActive = active; }

int EntityTooltipWidget::popFollowRequest()
{
    int id = _pendingFollowId;
    _pendingFollowId = -1;
    return id;
}

bool EntityTooltipWidget::isFollowButtonHovered() const { return _btnHovered; }

bool EntityTooltipWidget::_isPlayerCase() const
{
    return _selection.type == SelectionFinder::EntityType::Player && _world &&
           _world->playerExists(_selection.id);
}

void EntityTooltipWidget::_rebuild() const
{
    _tooltip.clearLines();

    if (!_world || _selection.type == SelectionFinder::EntityType::None) return;

    // Reserve space for the button at the bottom of the panel (player case only)
    float extraPad = _isPlayerCase() ? BTN_H + BTN_MARGIN * 2.0f : 0.0f;
    _tooltip.setExtraBottomPadding(extraPad);
    // Ensure panel is at least as wide as the button + side padding
    _tooltip.setMinWidth(_isPlayerCase() ? _btnW + 20.0f * 2.0f : 0.0f);

    switch (_selection.type) {
        case SelectionFinder::EntityType::Tile: {
            const Resources& res = _world->at(_selection.tileX, _selection.tileY);
            if (res.isEmpty()) {
                _tooltip.addColoredLine(
                    {I18n::get(I18n::Key::LABEL_TILE), I18n::get(I18n::Key::TILE_EMPTY)},
                    {TILE_COLOR, TEXT_COLOR});
            } else {
                _tooltip.addLine(I18n::get(I18n::Key::TILE_HEADER), TILE_COLOR);
                for (int i = 0; i < 7; i++) {
                    if (res[i] <= 0) continue;
                    _tooltip.addLine(
                        std::string("  ") + I18n::resourceName(i) + ": " + std::to_string(res[i]),
                        TEXT_COLOR);
                }
            }
            break;
        }

        case SelectionFinder::EntityType::Player: {
            if (!_world->playerExists(_selection.id)) return;
            const Player& p = _world->players.at(_selection.id);
            Color teamColor = _colorFunc ? _colorFunc(p.team) : WHITE;
            _tooltip.addColoredLine(
                {I18n::get(I18n::Key::LABEL_PLAYER), " #" + std::to_string(p.id)},
                {PLAYER_COLOR, TEXT_COLOR});
            _tooltip.addColoredLine({I18n::get(I18n::Key::PLAYER_TEAM), p.team},
                                    {TEXT_COLOR, teamColor});
            _tooltip.addLine(
                std::string(I18n::get(I18n::Key::PLAYER_LEVEL)) + std::to_string(p.level),
                TEXT_COLOR);
            if (p.inventory.isEmpty()) {
                _tooltip.addLine(I18n::get(I18n::Key::PLAYER_INVENTORY_EMPTY), TEXT_COLOR);
            } else {
                _tooltip.addLine(I18n::get(I18n::Key::PLAYER_INVENTORY), TEXT_COLOR);
                for (int i = 0; i < 7; i++) {
                    if (p.inventory[i] <= 0) continue;
                    _tooltip.addLine(std::string("    ") + I18n::resourceName(i) + ": " +
                                         std::to_string(p.inventory[i]),
                                     TEXT_COLOR);
                }
            }
            break;
        }

        case SelectionFinder::EntityType::Egg: {
            if (!_world->eggExists(_selection.id)) return;
            const Egg& egg = _world->eggs.at(_selection.id);
            Color teamColor = _colorFunc ? _colorFunc(egg.team) : WHITE;
            _tooltip.addColoredLine(
                {I18n::get(I18n::Key::LABEL_EGG), " #" + std::to_string(egg.id)},
                {EGG_COLOR, TEXT_COLOR});
            _tooltip.addColoredLine({I18n::get(I18n::Key::EGG_TEAM), egg.team},
                                    {TEXT_COLOR, teamColor});
            break;
        }

        default:
            break;
    }
}

void EntityTooltipWidget::_positionButton() const
{
    Rectangle tb = _tooltip.getLastBounds();
    if (tb.width <= 0.0f) return;

    // Grow width to fit whichever label is longer, with horizontal padding.
    // Measure both so the button doesn't resize when toggling between the two labels.
    int mFollow = TextRenderer::measure(I18n::get(I18n::Key::BTN_FOLLOW), _scaledFontSize);
    int mStop = TextRenderer::measure(I18n::get(I18n::Key::BTN_STOP_FOLLOW), _scaledFontSize);
    _btnW = std::max(BTN_W, static_cast<float>(std::max(mFollow, mStop)) + 24.0f);

    float btnX = tb.x + (tb.width - _btnW) / 2.0f;
    float btnY = tb.y + tb.height - BTN_H - BTN_MARGIN;
    _followBtn.setPosition(btnX, btnY).setSize(_btnW, BTN_H);
}

bool EntityTooltipWidget::handleInput()
{
    _btnHovered = false;
    if (!_isPlayerCase()) return false;

    _positionButton();

    bool stopping = _followActive;
    _followBtn.setLabel(I18n::get(stopping ? I18n::Key::BTN_STOP_FOLLOW : I18n::Key::BTN_FOLLOW))
        .setNormalColor(stopping ? STOP_NORMAL : FOLLOW_NORMAL)
        .setHoverColor(stopping ? STOP_HOVER : FOLLOW_HOVER);

    _followBtn.handleInput();
    _btnHovered = CheckCollisionPointRec(GetMousePosition(), _followBtn.getBounds());

    if (_followBtn.wasClicked()) _pendingFollowId = _followActive ? -2 : _selection.id;

    return false;  // non-blocking: raycasts still fire (caller checks isFollowButtonHovered())
}

void EntityTooltipWidget::draw(int scaledFontSize) const
{
    if (_selection.type == SelectionFinder::EntityType::None) return;

    _scaledFontSize = scaledFontSize;
    _rebuild();
    _tooltip.draw(scaledFontSize);

    if (_isPlayerCase()) {
        _positionButton();
        _followBtn.draw(scaledFontSize);
    }
}
