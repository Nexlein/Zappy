#include "HudWidget.hpp"

#include <algorithm>
#include <unordered_map>

#include "I18n.hpp"
#include "TooltipRenderer.hpp"

void HudWidget::setWorld(const WorldState* world) { _world = world; }
void HudWidget::setDevMode(bool dev, int port, const std::string& machine)
{
    _devMode = dev;
    _devPort = port;
    _devMachine = machine;
}
void HudWidget::setTimeUnit(int timeUnit) { _timeUnit = timeUnit; }
void HudWidget::setServerUptime(unsigned int uptime) { _serverUptimeSeconds = uptime; }
void HudWidget::setTeamColorFunc(std::function<Color(const std::string&)> func)
{
    _colorFunc = std::move(func);
}

bool HudWidget::handleInput() { return false; }  // HUD is not interactive

void HudWidget::draw(int scaledFontSize) const
{
    if (!_world) return;

    Color bgColor = {20, 25, 35, 220};
    Color borderColor = {60, 70, 90, 200};
    Color accentColor = {210, 220, 240, 255};

    std::string mapText = std::string(I18n::get(I18n::Key::HUD_MAP)) +
                          std::to_string(_world->width) + "x" + std::to_string(_world->height);

    std::string uptimeText;
    if (_serverUptimeSeconds == 0) {
        uptimeText = I18n::get(I18n::Key::HUD_UPTIME_UNKNOWN);
    } else {
        int uptimeHours = _serverUptimeSeconds / 3600;
        int uptimeMinutes = (_serverUptimeSeconds % 3600) / 60;
        int uptimeSeconds = _serverUptimeSeconds % 60;
        uptimeText = I18n::get(I18n::Key::HUD_UPTIME_PREFIX);
        if (uptimeHours > 0)
            uptimeText += std::to_string(uptimeHours) + I18n::get(I18n::Key::HUD_UPTIME_H);
        if (uptimeMinutes > 0 || uptimeHours > 0)
            uptimeText += std::to_string(uptimeMinutes) + I18n::get(I18n::Key::HUD_UPTIME_M);
        uptimeText += std::to_string(uptimeSeconds) + I18n::get(I18n::Key::HUD_UPTIME_S);
    }

    std::unordered_map<std::string, int> teamPlayerCounts;
    for (const auto& teamName : _world->teams) teamPlayerCounts[teamName] = 0;
    for (const auto& [id, player] : _world->players) teamPlayerCounts[player.team]++;

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
            std::string(I18n::get(I18n::Key::HUD_TIME_UNIT)) + std::to_string(_timeUnit),
            accentColor);
        builder.addLine(std::string(I18n::get(I18n::Key::HUD_PORT)) + std::to_string(_devPort),
                        accentColor);
        builder.addLine(std::string(I18n::get(I18n::Key::HUD_MACHINE)) + _devMachine, accentColor);
    }

    builder.addLine(mapText, accentColor).addLine(uptimeText, accentColor);

    for (size_t i = 0; i < std::min(sortedTeams.size(), size_t(5)); i++) {
        const auto& [teamName, playerCount] = sortedTeams[i];
        std::string teamLine = teamName + ": " + std::to_string(playerCount);
        builder.addLine(teamLine, _colorFunc ? _colorFunc(teamName) : WHITE);
    }

    builder.setBackgroundColor(bgColor)
        .setBackgroundAlpha(180)
        .setBorderColor(borderColor)
        .setBorderThickness(2)
        .setPadding(10)
        .setFontSize(scaledFontSize)
        .setAnchor(TooltipRenderer::Anchor::TopLeft)
        .draw({10.0f, 10.0f});
}
