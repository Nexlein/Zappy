/// @file WinScreen.cpp

#include "WinScreen.hpp"

#include <cstdint>
#include <string>

#include "I18n.hpp"

static constexpr float PANEL_MIN_W = 480.0f;
static constexpr float BTN_W = 140.0f;
static constexpr float BTN_H = 40.0f;
static constexpr float BTN_MARGIN = 20.0f;  // gap between last line and button
static constexpr Color OVERLAY_COLOR = {0, 0, 0, 178};
static constexpr Color PANEL_BORDER = {60, 70, 90, 200};
static constexpr Color ACCENT = {210, 220, 240, 255};
static constexpr Color TITLE_COLOR = {255, 220, 80, 255};

WinScreen::WinScreen()
{
    _panel.setAnchor(TooltipWidget::Anchor::Center)
        .setBackgroundColor({20, 25, 35, 255})
        .setBackgroundAlpha(178)
        .setBorderColor(PANEL_BORDER)
        .setBorderThickness(2)
        .setPadding(18)
        .setMinWidth(PANEL_MIN_W)
        .setTextAlign(TooltipWidget::TextAlign::Center)
        .setExtraBottomPadding(BTN_H + BTN_MARGIN * 2);

    _quitBtn.setSize(120.0f, 36.0f)
        .setAnchor(ButtonWidget::Anchor::None)  // positioned relative to panel each frame
        .setRoundness(0.3f)
        .setBorderColor(PANEL_BORDER)
        .setBorderThickness(2.0f);
}

void WinScreen::setWinner(const std::string& team, Color color)
{
    _winnerTeam = team;
    _winnerColor = color;
}

void WinScreen::setDuration(int seconds, int64_t ticks)
{
    _durationSeconds = seconds;
    _durationTicks = ticks;
}

std::string WinScreen::_formatDuration(unsigned int seconds)
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

void WinScreen::_rebuildPanel(int scaledFontSize) const
{
    (void)scaledFontSize;
    _panel.clearLines();

    // Title line
    _panel.addLine(I18n::get(I18n::Key::WIN_GAME_OVER), TITLE_COLOR);

    // "Team X won!"
    _panel.addColoredLine({_winnerTeam, I18n::get(I18n::Key::WIN_TEAM_WON)},
                          {_winnerColor, ACCENT});

    // Duration line
    if (_durationSeconds < 0) {
        _panel.addLine(I18n::get(I18n::Key::WIN_DURATION_UNKNOWN), ACCENT);
    } else {
        std::string durLine = std::string(I18n::get(I18n::Key::WIN_DURATION)) +
                              _formatDuration(static_cast<unsigned int>(_durationSeconds)) + " (" +
                              std::to_string(_durationTicks) +
                              I18n::get(I18n::Key::WIN_TICKS) + ")";
        _panel.addLine(durLine, ACCENT);
    }
}

bool WinScreen::handleInput()
{
    _quitBtn.handleInput();
    if (_quitBtn.wasClicked()) _quitRequested = true;
    return true;  // overlay always consumes all input
}

void WinScreen::draw(int scaledFontSize) const
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Full-screen dim
    DrawRectangle(0, 0, sw, sh, OVERLAY_COLOR);

    _rebuildPanel(scaledFontSize);
    _panel.draw(scaledFontSize);

    // Position Quit button in the reserved space at the bottom of the panel
    _quitBtn.setLabel(I18n::get(I18n::Key::WIN_QUIT));
    Rectangle panelBounds = _panel.getLastBounds();
    float btnX = panelBounds.x + (panelBounds.width - BTN_W) / 2.0f;
    float btnY = panelBounds.y + panelBounds.height - BTN_H - BTN_MARGIN;
    _quitBtn.setPosition(btnX, btnY).setSize(BTN_W, BTN_H);
    _quitBtn.draw(scaledFontSize);
}
