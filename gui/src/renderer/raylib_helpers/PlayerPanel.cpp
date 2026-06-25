#include "PlayerPanel.hpp"

#include <algorithm>

void PlayerPanel::setWorld(const WorldState* world) { _world = world; }

void PlayerPanel::draw(int scaledFontSize) const
{
    if (!_world) return;

    if (!_isOpen) return;

    Rectangle contentBox = _getPanelBounds();

    Color bgColor = {30, 30, 40, 220};
    Color borderColor = {80, 80, 90, 255};
    Color textColor = {240, 240, 240, 255};

    DrawRectangleRec(contentBox, bgColor);
    DrawRectangleLinesEx(contentBox, 2, borderColor);

    int y = static_cast<int>(contentBox.y) + 10;
    int x = static_cast<int>(contentBox.x) + 20;

    const char* title = "Player List";
    DrawText(title,
             static_cast<int>(contentBox.x + contentBox.width / 2 -
                              MeasureText(title, scaledFontSize) / 2),
             y, scaledFontSize, textColor);
    y += ROW_HEIGHT;

    std::vector<const Player*> players;
    for (const auto& [id, player] : _world->players) players.push_back(&player);

    std::sort(players.begin(), players.end(), [](const Player* a, const Player* b) {
        if (a->team != b->team) return a->team < b->team;
        return a->id < b->id;
    });

    for (const Player* p : players) {
        Color tColor = _colorFunc ? _colorFunc(p->team) : WHITE;
        std::string txt =
            "P" + std::to_string(p->id) + " (Lvl " + std::to_string(p->level) + ") - " + p->team;
        DrawText(txt.c_str(), x, y, scaledFontSize, tColor);
        y += ROW_HEIGHT;
    }
}

bool PlayerPanel::handleInput()
{
    if (IsKeyPressed(KEY_TAB)) _isOpen = !_isOpen;

    if (!_isOpen) return false;

    Vector2 mousePos = GetMousePosition();
    Rectangle contentBox = _getPanelBounds();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePos, contentBox)) {
            int relY = static_cast<int>(mousePos.y - contentBox.y - 10 - ROW_HEIGHT);
            int index = relY / ROW_HEIGHT;

            std::vector<const Player*> players;
            for (const auto& [id, player] : _world->players) players.push_back(&player);

            std::sort(players.begin(), players.end(), [](const Player* a, const Player* b) {
                if (a->team != b->team) return a->team < b->team;
                return a->id < b->id;
            });

            if (index >= 0 && index < static_cast<int>(players.size())) {
                _pendingSelection = SelectionFinder::Selection{SelectionFinder::EntityType::Player,
                                                               players[index]->id, -1, -1};
                return true;
            }
        }
    }
    return false;
}

std::optional<SelectionFinder::Selection> PlayerPanel::getPendingSelection()
{
    auto val = _pendingSelection;
    _pendingSelection = std::nullopt;
    return val;
}

Rectangle PlayerPanel::_getPanelBounds() const
{
    float screenW = static_cast<float>(GetScreenWidth());
    float screenH = static_cast<float>(GetScreenHeight());
    float h = static_cast<float>(_getContentHeight());
    return {screenW / 2.0f - PANEL_WIDTH / 2.0f, screenH / 2.0f - h / 2.0f,
            static_cast<float>(PANEL_WIDTH), h};
}

int PlayerPanel::_getContentHeight() const
{
    int count = _world ? static_cast<int>(_world->players.size()) : 0;
    return std::max(50, count * ROW_HEIGHT + ROW_HEIGHT + 20);
}
