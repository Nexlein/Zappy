/// @file EntityTooltipWidget.hpp
/// @brief Tooltip widget for selected entities (tile, player, egg).
///
/// Player case embeds a Follow/Stop Follow button inside the panel.
/// Call setFollowActive() each frame so the button label stays in sync.

#pragma once

#include <functional>
#include <string>

#include "ButtonWidget.hpp"
#include "IWidget.hpp"
#include "SelectionFinder.hpp"
#include "TooltipWidget.hpp"
#include "core/WorldState.hpp"
#include "raylib.h"

class EntityTooltipWidget : public IWidget {
    public:
    EntityTooltipWidget();
    ~EntityTooltipWidget() override = default;

    void setSelection(const SelectionFinder::Selection& sel);
    void setWorld(const WorldState* world);
    void setTeamColorFunc(std::function<Color(const std::string&)> func);
    /// @brief Tells the widget whether follow mode is currently active (affects button
    /// label/color).
    void setFollowActive(bool active);

    /// @brief Returns and clears the pending follow target player id.
    /// Positive id = start follow. -2 = stop follow requested. -1 = no request.
    int popFollowRequest();

    /// @brief True when the follow button is hovered — use to block raycasts.
    bool isFollowButtonHovered() const;

    void draw(int scaledFontSize) const override;
    bool handleInput() override;

    private:
    const WorldState* _world = nullptr;
    SelectionFinder::Selection _selection;
    std::function<Color(const std::string&)> _colorFunc;
    bool _followActive = false;

    int _pendingFollowId = -1;
    mutable int _scaledFontSize = 18;

    mutable TooltipWidget _tooltip;
    mutable ButtonWidget _followBtn;
    mutable bool _btnHovered = false;
    mutable float _btnW = BTN_W;

    static constexpr float BTN_W = 120.0f;
    static constexpr float BTN_H = 36.0f;
    static constexpr float BTN_MARGIN = 10.0f;  // gap inside panel below last line

    void _rebuild() const;
    bool _isPlayerCase() const;
    void _positionButton() const;
};
