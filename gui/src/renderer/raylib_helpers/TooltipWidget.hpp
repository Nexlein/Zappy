/// @file TooltipWidget.hpp
/// @brief Stateful tooltip panel component implementing IWidget.

#pragma once

#include <string>
#include <vector>

#include "IWidget.hpp"
#include "raylib.h"

/// @brief A configurable, stateful tooltip panel.
///
/// Wraps TooltipRenderer::Builder to provide an IWidget-compatible component.
/// Content is built once via the fluent API and drawn each frame until changed.
/// Never handles input (handleInput always returns false) — it is display-only.
///
/// Positioning modes:
///   - Anchor::None + setPosition(): absolute screen coordinates.
///   - Any other Anchor: auto-positioned relative to screen edges with a margin.
///   - setScreenCenter(): centers the panel on screen each frame.
class TooltipWidget : public IWidget {
    public:
    enum class Anchor {
        None,
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        Center,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight,
    };

    TooltipWidget() = default;
    ~TooltipWidget() override = default;

    // ── Content ────────────────────────────────────────────────────────────
    /// @brief Adds a single-color text line.
    TooltipWidget& addLine(const std::string& text, Color color = WHITE);

    /// @brief Adds a line with multiple colored segments (inline spans).
    TooltipWidget& addColoredLine(const std::vector<std::string>& segments,
                                  const std::vector<Color>& colors);

    /// @brief Removes all lines.
    TooltipWidget& clearLines();

    // ── Geometry ───────────────────────────────────────────────────────────
    /// @brief Absolute position. Sets anchor to None.
    TooltipWidget& setPosition(float x, float y);

    /// @brief Auto-positioning relative to screen edges.
    /// @param margin Pixel gap from the anchored edge.
    TooltipWidget& setAnchor(Anchor anchor, float margin = 10.0f);

    /// @brief Override minimum panel width (0 = fit to content).
    TooltipWidget& setMinWidth(float minWidth);

    // ── Appearance ─────────────────────────────────────────────────────────
    TooltipWidget& setBackgroundColor(Color color);
    TooltipWidget& setBackgroundAlpha(unsigned char alpha);
    TooltipWidget& setBorderColor(Color color);
    TooltipWidget& setBorderThickness(int thickness);
    TooltipWidget& setPadding(int padding);

    // ── IWidget ────────────────────────────────────────────────────────────
    void draw(int scaledFontSize) const override;

    /// @brief Always returns false — tooltip panels are display-only.
    bool handleInput() override { return false; }

    /// @brief Returns the bounding rectangle for the last drawn frame.
    /// Useful for positioning sibling widgets (e.g. a button below the panel).
    Rectangle getLastBounds() const { return _lastBounds; }

    private:
    struct Line {
        std::vector<std::string> segments;
        std::vector<Color> colors;
    };

    std::vector<Line> _lines;

    float _x = 0.0f;
    float _y = 0.0f;
    Anchor _anchor = Anchor::None;
    float _margin = 10.0f;
    float _minWidth = 0.0f;

    Color _bgColor = {20, 25, 35, 220};
    unsigned char _bgAlpha = 220;
    Color _borderColor = {60, 70, 90, 200};
    int _borderThickness = 2;
    int _padding = 12;

    mutable Rectangle _lastBounds = {};

    Vector2 _resolvePosition(float boxWidth, float boxHeight) const;
};
