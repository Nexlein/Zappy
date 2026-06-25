#include "WinScreen.hpp"

#include <cstdint>
#include <string>

#include "I18n.hpp"

static constexpr float PANEL_W = 480.0f;
static constexpr float PANEL_H = 200.0f;
static constexpr float ROUNDNESS = 0.2f;
static constexpr int SEGMENTS = 8;
static constexpr float BORDER_THICKNESS = 2.0f;

static constexpr Color OVERLAY_COLOR = {0, 0, 0, 178};  // ~70% opacity black vignette
static constexpr Color BG_COLOR = {20, 25, 35, 178};    // 70% opacity panel
static constexpr Color BORDER_COLOR = {60, 70, 90, 200};
static constexpr Color ACCENT_COLOR = {210, 220, 240, 255};
static constexpr Color TITLE_COLOR = {255, 220, 80, 255};

static constexpr Color BTN_NORMAL = {60, 70, 90, 220};
static constexpr Color BTN_HOVER = {100, 120, 160, 240};
static constexpr Color BTN_TEXT = {210, 220, 240, 255};

std::string WinScreen::_formatUptime(unsigned int seconds)
{
    unsigned int h = seconds / 3600;
    unsigned int m = (seconds % 3600) / 60;
    unsigned int s = seconds % 60;
    std::string out;
    if (h > 0) out += std::to_string(h) + "h ";
    if (m > 0 || h > 0) out += std::to_string(m) + "m ";
    out += std::to_string(s) + "s";
    return out;
}

bool WinScreen::_drawButton(const char* label, Rectangle rect, int fontSize)
{
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    DrawRectangleRounded(rect, 0.3f, 8, hovered ? BTN_HOVER : BTN_NORMAL);
    DrawRectangleRoundedLines(rect, 0.3f, 8, BORDER_THICKNESS, BORDER_COLOR);

    int tw = MeasureText(label, fontSize);
    int tx = static_cast<int>(rect.x + (rect.width - tw) / 2.0f);
    int ty = static_cast<int>(rect.y + (rect.height - fontSize) / 2.0f);
    DrawText(label, tx, ty, fontSize, BTN_TEXT);

    return clicked;
}

bool WinScreen::draw(const std::string& winnerTeam, Color winnerColor, int teamJoinSeconds,
                     int64_t teamJoinTicks, unsigned int serverUptimeSeconds, int scaledFontSize)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Full-screen dim
    DrawRectangle(0, 0, sw, sh, OVERLAY_COLOR);

    // Centered panel
    float px = (sw - PANEL_W) / 2.0f;
    float py = (sh - PANEL_H) / 2.0f;
    Rectangle panel = {px, py, PANEL_W, PANEL_H};

    DrawRectangleRounded(panel, ROUNDNESS, SEGMENTS, BG_COLOR);
    DrawRectangleRoundedLines(panel, ROUNDNESS, SEGMENTS, BORDER_THICKNESS, BORDER_COLOR);

    int bigFont = scaledFontSize + 6;
    int pad = 18;

    // Title
    const char* title = I18n::get(I18n::Key::WIN_GAME_OVER);
    int titleW = MeasureText(title, bigFont);
    DrawText(title, static_cast<int>(px + (PANEL_W - titleW) / 2.0f), static_cast<int>(py + pad),
             bigFont, TITLE_COLOR);

    // "Team X won"
    std::string teamLine = winnerTeam + I18n::get(I18n::Key::WIN_TEAM_WON);
    int teamFont = scaledFontSize;
    int teamW = MeasureText(teamLine.c_str(), teamFont);
    DrawText(teamLine.c_str(), static_cast<int>(px + (PANEL_W - teamW) / 2.0f),
             static_cast<int>(py + pad + bigFont + 10), teamFont, winnerColor);

    // Duration = serverUptime(stu) - teamJoinTime(gtt). Show "..." until gtt reply arrives.
    std::string durLine;
    if (teamJoinSeconds < 0 || serverUptimeSeconds == 0) {
        durLine = I18n::get(I18n::Key::WIN_DURATION_UNKNOWN);
    } else {
        unsigned int duration =
            serverUptimeSeconds > static_cast<unsigned int>(teamJoinSeconds)
                ? serverUptimeSeconds - static_cast<unsigned int>(teamJoinSeconds)
                : 0;
        durLine = std::string(I18n::get(I18n::Key::WIN_DURATION)) + _formatUptime(duration) + " (" +
                  std::to_string(teamJoinTicks) + I18n::get(I18n::Key::WIN_TICKS) + ")";
    }
    int durW = MeasureText(durLine.c_str(), scaledFontSize);
    DrawText(durLine.c_str(), static_cast<int>(px + (PANEL_W - durW) / 2.0f),
             static_cast<int>(py + pad + bigFont + 10 + teamFont + 10), scaledFontSize,
             ACCENT_COLOR);

    // Quit button
    float btnW = 120.0f;
    float btnH = 36.0f;
    Rectangle btnRect = {px + (PANEL_W - btnW) / 2.0f, py + PANEL_H - btnH - pad, btnW, btnH};
    return _drawButton(I18n::get(I18n::Key::WIN_QUIT), btnRect, scaledFontSize);
}
