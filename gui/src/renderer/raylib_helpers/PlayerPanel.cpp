#include "PlayerPanel.hpp"

#include <algorithm>

void PlayerPanel::setWorld(const WorldState* world) { _world = world; }

bool PlayerPanel::handleInput()
{
    if (IsKeyPressed(KEY_TAB)) _isOpen = !_isOpen;

    if (!_isOpen || !_world) return false;

    _background.clearLines();
    _background.addLine("Player List", WHITE);
    _background.setAnchor(TooltipWidget::Anchor::Center);
    _background.setMinWidth(PANEL_WIDTH);
    _background.setTextAlign(TooltipWidget::TextAlign::Center);

    std::vector<const Player*> players;
    for (const auto& [id, player] : _world->players) players.push_back(&player);

    std::sort(players.begin(), players.end(), [](const Player* a, const Player* b) {
        if (a->team != b->team) return a->team < b->team;
        return a->id < b->id;
    });

    _background.setExtraBottomPadding(players.size() * ROW_HEIGHT + 10);

    Rectangle bgBounds = _background.getLastBounds();

    float startY = bgBounds.y + 40.0f;

    _playerButtons.clear();
    for (const Player* p : players) {
        Color tColor = _colorFunc ? _colorFunc(p->team) : WHITE;
        std::string txt =
            "P" + std::to_string(p->id) + " (Lvl " + std::to_string(p->level) + ") - " + p->team;

        ButtonWidget btn;
        btn.setLabel(txt)
            .setPosition(bgBounds.x + 10, startY)
            .setSize(bgBounds.width > 20 ? bgBounds.width - 20 : PANEL_WIDTH - 20, ROW_HEIGHT)
            .setNormalColor({0, 0, 0, 0})
            .setHoverColor({100, 120, 160, 200})
            .setTextColor(tColor)
            .setBorderThickness(0)
            .setRoundness(0.2f);

        int pId = p->id;
        btn.setOnClick([this, pId]() {
            _pendingSelection =
                SelectionFinder::Selection{SelectionFinder::EntityType::Player, pId, -1, -1};
        });

        _playerButtons.push_back(btn);
        startY += ROW_HEIGHT;
    }

    bool consumed = false;
    for (auto& btn : _playerButtons) {
        if (btn.handleInput()) consumed = true;
    }

    if (CheckCollisionPointRec(GetMousePosition(), bgBounds)) consumed = true;

    return consumed;
}

std::optional<SelectionFinder::Selection> PlayerPanel::getPendingSelection()
{
    auto val = _pendingSelection;
    _pendingSelection = std::nullopt;
    return val;
}

void PlayerPanel::draw(int scaledFontSize) const
{
    if (!_isOpen || !_world) return;

    _background.draw(scaledFontSize);

    for (const auto& btn : _playerButtons) btn.draw(scaledFontSize);
}
