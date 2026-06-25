#pragma once

#include <optional>
#include <vector>

#include "IWidget.hpp"

/**
 * @brief Self-contained speed slider widget.
 *
 * Encapsulates all state, input handling, and drawing for the server-speed
 * slider panel displayed at the bottom-left of the screen. Matches the HUD
 * visual style (same rounded panel, colors, and font scaling).
 *
 * Discrete speed steps: 1, 2, 3, 4, then 5 to 200 in steps of 5.
 */
class SpeedSlider : public IWidget {
    public:
    static constexpr int PANEL_WIDTH = 220;
    static constexpr int PANEL_HEIGHT = 58;

    static const std::vector<int> STEPS;

    /**
     * @brief Snaps to the nearest step index matching serverTimeUnit.
     * No-op if already initialized or serverTimeUnit is 0.
     */
    void syncFromServer(int serverTimeUnit);

    /**
     * @brief Processes mouse input.
     * @return true if the slider consumed the input.
     */
    bool handleInput() override;

    /**
     * @brief Returns and clears any pending speed change.
     */
    std::optional<int> getPendingSpeedChange();

    /**
     * @brief Draws the slider panel at the bottom-left of the screen.
     * @param scaledFontSize Pre-scaled font size matching the HUD.
     */
    void draw(int scaledFontSize) const override;

    int currentSpeed() const { return STEPS[_index]; }

    private:
    int _index = 19;  // default: 100 (index 19 in STEPS)
    bool _dragging = false;
    bool _initialized = false;
    std::optional<int> _pendingSpeed = std::nullopt;

    static float _trackX(float panelX);
    static float _trackY(float panelY, float panelH);
    static float _trackW(float panelW);
};
