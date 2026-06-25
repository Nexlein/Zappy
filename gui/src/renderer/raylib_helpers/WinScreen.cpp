/// @file WinScreen.cpp

#include "WinScreen.hpp"

#include <cstdint>
#include <string>

#include "I18n.hpp"

static constexpr float PANEL_MIN_W = 480.0f;
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
        .setMinWidth(PANEL_MIN_W);

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

void WinScreen::setDuration(int teamJoinSeconds, int64_t teamJoinTicks, unsigned int gameEndUptime)
{
    _teamJoinSeconds = teamJoinSeconds;
    _teamJoinTicks = teamJoinTicks;
    _gameEndUptime = gameEndUptime;
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
    if (_teamJoinSeconds < 0 || _gameEndUptime == 0) {
        _panel.addLine(I18n::get(I18n::Key::WIN_DURATION_UNKNOWN), ACCENT);
    } else {
        unsigned int elapsed = _gameEndUptime > static_cast<unsigned int>(_teamJoinSeconds)
                                   ? _gameEndUptime - static_cast<unsigned int>(_teamJoinSeconds)
                                   : 0u;
        std::string durLine = std::string(I18n::get(I18n::Key::WIN_DURATION)) +
                              _formatDuration(elapsed) + " (" + std::to_string(_teamJoinTicks) +
                              I18n::get(I18n::Key::WIN_TICKS) + ")";
        _panel.addLine(durLine, ACCENT);
    }

    // Empty line for visual spacing before the button
    _panel.addLine("", ACCENT);
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

    // Position Quit button at the bottom of the panel
    Rectangle panelBounds = _panel.getLastBounds();
    float btnW = 120.0f;
    float btnH = 36.0f;
    float btnX = panelBounds.x + (panelBounds.width - btnW) / 2.0f;
    float btnY = panelBounds.y + panelBounds.height - btnH - 14.0f;
    _quitBtn.setPosition(btnX, btnY).setSize(btnW, btnH);
    _quitBtn.draw(scaledFontSize);
}
