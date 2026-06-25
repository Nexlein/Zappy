/// @file HudWidget.hpp
/// @brief HUD panel displaying general game information.

#pragma once

#include <functional>
#include <string>

#include "IWidget.hpp"
#include "core/WorldState.hpp"

/// @brief HUD widget for general information (map, uptime, fps, etc).
class HudWidget : public IWidget {
    public:
    HudWidget() = default;
    ~HudWidget() override = default;

    void setWorld(const WorldState* world);
    void setDevMode(bool dev, int port, const std::string& machine);
    void setTimeUnit(int timeUnit);
    void setServerUptime(unsigned int uptime);
    void setTeamColorFunc(std::function<Color(const std::string&)> func);

    void draw(int scaledFontSize) const override;
    bool handleInput() override;

    private:
    const WorldState* _world = nullptr;
    bool _devMode = false;
    int _devPort = -1;
    std::string _devMachine;
    int _timeUnit = 0;
    unsigned int _serverUptimeSeconds = 0;
    std::function<Color(const std::string&)> _colorFunc;
};
