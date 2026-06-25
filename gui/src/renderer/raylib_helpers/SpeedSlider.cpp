#include "SpeedSlider.hpp"

#include <cmath>
#include <string>

#include "I18n.hpp"

const std::vector<int> SpeedSlider::STEPS = {1,   2,   3,   4,   5,   10,  15,  20,  25,  30,  35,
                                             40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,
                                             95,  100, 105, 110, 115, 120, 125, 130, 135, 140, 145,
                                             150, 155, 160, 165, 170, 175, 180, 185, 190, 195, 200};

static constexpr float ROUNDNESS = 0.2f;
static constexpr int SEGMENTS = 8;
static constexpr float BORDER_THICKNESS = 2.0f;
static constexpr float TRACK_H = 4.0f;
static constexpr float HANDLE_RADIUS = 6.0f;

static constexpr Color BG_COLOR = {20, 25, 35, 180};
static constexpr Color BORDER_COLOR = {60, 70, 90, 200};
static constexpr Color ACCENT_COLOR = {210, 220, 240, 255};
static constexpr Color TRACK_BG_COLOR = {60, 70, 90, 220};
static constexpr Color TRACK_FILL_COLOR = {100, 160, 255, 255};
static constexpr Color HANDLE_DRAG_COLOR = {255, 220, 80, 255};

float SpeedSlider::_trackX(float panelX) { return panelX + 10.0f; }
float SpeedSlider::_trackY(float panelY, float panelH) { return panelY + panelH * 0.75f; }
float SpeedSlider::_trackW(float panelW) { return panelW - 20.0f; }

void SpeedSlider::syncFromServer(int serverTimeUnit)
{
    if (_initialized || serverTimeUnit <= 0) return;

    int best = 0;
    int bestDiff = std::abs(STEPS[0] - serverTimeUnit);
    for (int i = 1; i < static_cast<int>(STEPS.size()); i++) {
        int diff = std::abs(STEPS[i] - serverTimeUnit);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = i;
        }
    }
    _index = best;
    _initialized = true;
}

std::optional<int> SpeedSlider::handleInput()
{
    int sh = GetScreenHeight();
    float panelX = 10.0f;
    float panelY = static_cast<float>(sh - PANEL_HEIGHT - 10);
    float panelW = static_cast<float>(PANEL_WIDTH);
    float panelH = static_cast<float>(PANEL_HEIGHT);

    float tx = _trackX(panelX);
    float ty = _trackY(panelY, panelH);
    float tw = _trackW(panelW);

    float t = static_cast<float>(_index) / static_cast<float>(STEPS.size() - 1);
    float handleX = tx + tw * t;

    Rectangle panelRect = {panelX, panelY, panelW, panelH};
    Rectangle handleHit = {handleX - HANDLE_RADIUS * 1.5f, ty - HANDLE_RADIUS * 1.5f,
                           HANDLE_RADIUS * 3.0f, HANDLE_RADIUS * 3.0f};

    Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        (CheckCollisionPointRec(mouse, panelRect) || CheckCollisionPointRec(mouse, handleHit)))
        _dragging = true;

    if (_dragging) {
        float frac = (mouse.x - tx) / tw;
        frac = frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac);
        _index = static_cast<int>(frac * static_cast<float>(STEPS.size() - 1) + 0.5f);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            _dragging = false;
            return STEPS[_index];
        }
    }

    return std::nullopt;
}

void SpeedSlider::draw(int scaledFontSize) const
{
    int sh = GetScreenHeight();
    float panelX = 10.0f;
    float panelY = static_cast<float>(sh - PANEL_HEIGHT - 10);
    float panelW = static_cast<float>(PANEL_WIDTH);
    float panelH = static_cast<float>(PANEL_HEIGHT);

    Rectangle rect = {panelX, panelY, panelW, panelH};
    DrawRectangleRounded(rect, ROUNDNESS, SEGMENTS, BG_COLOR);
    DrawRectangleRoundedLines(rect, ROUNDNESS, SEGMENTS, BORDER_THICKNESS, BORDER_COLOR);

    std::string label =
        std::string(I18n::get(I18n::Key::SPEED_LABEL)) + std::to_string(STEPS[_index]);
    DrawText(label.c_str(), static_cast<int>(panelX + 10), static_cast<int>(panelY + 8),
             scaledFontSize, ACCENT_COLOR);

    float tx = _trackX(panelX);
    float ty = _trackY(panelY, panelH);
    float tw = _trackW(panelW);

    DrawRectangleRounded({tx, ty - TRACK_H / 2.0f, tw, TRACK_H}, 1.0f, 4, TRACK_BG_COLOR);

    float t = static_cast<float>(_index) / static_cast<float>(STEPS.size() - 1);
    float fillW = tw * t;
    if (fillW > 0.5f)
        DrawRectangleRounded({tx, ty - TRACK_H / 2.0f, fillW, TRACK_H}, 1.0f, 4, TRACK_FILL_COLOR);

    float handleX = tx + fillW;
    Color handleColor = _dragging ? HANDLE_DRAG_COLOR : ACCENT_COLOR;
    DrawCircle(static_cast<int>(handleX), static_cast<int>(ty), HANDLE_RADIUS, handleColor);
    DrawCircleLines(static_cast<int>(handleX), static_cast<int>(ty), HANDLE_RADIUS, BORDER_COLOR);
}
