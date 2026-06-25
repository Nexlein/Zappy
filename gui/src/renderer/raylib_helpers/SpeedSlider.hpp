#pragma once

#include <optional>
#include <vector>

#include "raylib.h"

/**
 * @brief Self-contained speed slider widget.
 *
 * Encapsulates all state, input handling, and drawing for the server-speed
 * slider panel displayed at the bottom-left of the screen. Matches the HUD
 * visual style (same rounded panel, colors, and font scaling).
 *
 * Discrete speed steps: 1, 2, 3, 4, then 5 to 200 in steps of 5.
 */
class SpeedSlider {
    public:
    static constexpr int PANEL_WIDTH = 220;
    static constexpr int PANEL_HEIGHT = 58;

    static const std::vector<int> STEPS;

    /**
     * @brief Snaps to the nearest step index matching serverTimeUnit.
     * No-op if already initialized or serverTimeUnit is 0.
     */
    void syncFromServer(int serverTimeUnit);

    /** @brief Resets slider state. Call on reconnect so it re-syncs from the new server. */
    void reset()
    {
        _initialized = false;
        _dragging = false;
    }

    /**
     * @brief Processes mouse input. Returns the new speed if the user released
     * the handle, std::nullopt otherwise.
     */
    std::optional<int> handleInput();

    /**
     * @brief Draws the slider panel at the bottom-left of the screen.
     * @param scaledFontSize Pre-scaled font size matching the HUD.
     */
    void draw(int scaledFontSize) const;

    int currentSpeed() const { return STEPS[_index]; }

    private:
    int _index = 23;  // default: 100 (index 23 in STEPS)
    bool _dragging = false;
    bool _initialized = false;

    static float _trackX(float panelX);
    static float _trackY(float panelY, float panelH);
    static float _trackW(float panelW);
};
