/// @file ButtonWidget.hpp
/// @brief Reusable clickable button component.

#pragma once

#include <functional>
#include <string>

#include "IWidget.hpp"
#include "raylib.h"

/// @brief A configurable, stateful clickable button.
///
/// Supports custom label, position, size, colors, corner roundness, and an optional
/// on-click callback. Hover state is tracked internally each frame.
/// Use wasClicked() to poll the result, or setOnClick() for a callback.
class ButtonWidget : public IWidget {
    public:
    /// @brief Positioning modes for automatic layout.
    enum class Anchor {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        Center,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
        /// @brief No anchor: position set directly via setPosition().
        None
    };

    ButtonWidget() = default;
    ~ButtonWidget() override = default;

    // ── Content ────────────────────────────────────────────────────────────
    ButtonWidget& setLabel(const std::string& label);

    // ── Geometry ───────────────────────────────────────────────────────────
    /// @brief Sets the button position directly (requires Anchor::None).
    ButtonWidget& setPosition(float x, float y);
    ButtonWidget& setSize(float width, float height);
    /// @brief Sets anchor: button is auto-positioned relative to screen edges.
    /// @param margin Pixel offset from the anchored edge.
    ButtonWidget& setAnchor(Anchor anchor, float margin = 10.0f);

    // ── Appearance ─────────────────────────────────────────────────────────
    ButtonWidget& setNormalColor(Color color);
    ButtonWidget& setHoverColor(Color color);
    ButtonWidget& setTextColor(Color color);
    ButtonWidget& setBorderColor(Color color);
    ButtonWidget& setBorderThickness(float thickness);
    ButtonWidget& setRoundness(float roundness);

    // ── Behaviour ──────────────────────────────────────────────────────────
    /// @brief Registers a callback invoked when the button is clicked.
    ButtonWidget& setOnClick(std::function<void()> callback);

    // ── IWidget ────────────────────────────────────────────────────────────
    void draw(int scaledFontSize) const override;
    /// @return true if the button is hovered (consumes input while hovered).
    bool handleInput() override;

    // ── State polling ──────────────────────────────────────────────────────
    /// @brief Returns true once per click, then resets.
    bool wasClicked();

    /// @brief Returns the bounding rectangle (after anchor resolution).
    Rectangle getBounds() const;

    private:
    std::string _label;

    float _x = 0.0f;
    float _y = 0.0f;
    float _width = 120.0f;
    float _height = 36.0f;

    Anchor _anchor = Anchor::None;
    float _margin = 10.0f;

    Color _normalColor = {60, 70, 90, 220};
    Color _hoverColor = {100, 120, 160, 240};
    Color _textColor = {210, 220, 240, 255};
    Color _borderColor = {60, 70, 90, 200};
    float _borderThickness = 2.0f;
    float _roundness = 0.3f;

    bool _hovered = false;
    bool _clicked = false;

    std::function<void()> _onClick;

    Rectangle _resolvedBounds() const;
};
