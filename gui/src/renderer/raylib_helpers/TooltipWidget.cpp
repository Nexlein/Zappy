/// @file TooltipWidget.cpp

#include "TooltipWidget.hpp"

#include <algorithm>
#include <stdexcept>

TooltipWidget& TooltipWidget::addLine(const std::string& text, Color color)
{
    _lines.push_back({{text}, {color}});
    return *this;
}

TooltipWidget& TooltipWidget::addColoredLine(const std::vector<std::string>& segments,
                                             const std::vector<Color>& colors)
{
    if (segments.size() != colors.size())
        throw std::invalid_argument("segments and colors size mismatch");
    _lines.push_back({segments, colors});
    return *this;
}

TooltipWidget& TooltipWidget::clearLines()
{
    _lines.clear();
    return *this;
}

TooltipWidget& TooltipWidget::setPosition(float x, float y)
{
    _x = x;
    _y = y;
    _anchor = Anchor::None;
    return *this;
}

TooltipWidget& TooltipWidget::setAnchor(Anchor anchor, float margin)
{
    _anchor = anchor;
    _margin = margin;
    return *this;
}

TooltipWidget& TooltipWidget::setMinWidth(float minWidth)
{
    _minWidth = minWidth;
    return *this;
}

TooltipWidget& TooltipWidget::setBackgroundColor(Color color)
{
    _bgColor = color;
    return *this;
}

TooltipWidget& TooltipWidget::setBackgroundAlpha(unsigned char alpha)
{
    _bgAlpha = alpha;
    return *this;
}

TooltipWidget& TooltipWidget::setBorderColor(Color color)
{
    _borderColor = color;
    return *this;
}

TooltipWidget& TooltipWidget::setBorderThickness(int thickness)
{
    _borderThickness = thickness;
    return *this;
}

TooltipWidget& TooltipWidget::setPadding(int padding)
{
    _padding = padding;
    return *this;
}

Vector2 TooltipWidget::_resolvePosition(float boxWidth, float boxHeight) const
{
    if (_anchor == Anchor::None) return {_x, _y};

    float sw = static_cast<float>(GetScreenWidth());
    float sh = static_cast<float>(GetScreenHeight());

    switch (_anchor) {
        case Anchor::TopLeft:
            return {_margin, _margin};
        case Anchor::TopCenter:
            return {(sw - boxWidth) / 2.0f, _margin};
        case Anchor::TopRight:
            return {sw - boxWidth - _margin, _margin};
        case Anchor::MiddleLeft:
            return {_margin, (sh - boxHeight) / 2.0f};
        case Anchor::Center:
            return {(sw - boxWidth) / 2.0f, (sh - boxHeight) / 2.0f};
        case Anchor::MiddleRight:
            return {sw - boxWidth - _margin, (sh - boxHeight) / 2.0f};
        case Anchor::BottomLeft:
            return {_margin, sh - boxHeight - _margin};
        case Anchor::BottomCenter:
            return {(sw - boxWidth) / 2.0f, sh - boxHeight - _margin};
        case Anchor::BottomRight:
            return {sw - boxWidth - _margin, sh - boxHeight - _margin};
        default:
            return {_x, _y};
    }
}

void TooltipWidget::draw(int scaledFontSize) const
{
    if (_lines.empty()) return;

    int lineSpacing = scaledFontSize / 4;
    int contentWidth = 0;
    int contentHeight = 0;

    for (const auto& line : _lines) {
        int lineWidth = 0;
        for (size_t i = 0; i < line.segments.size(); ++i)
            lineWidth += MeasureText(line.segments[i].c_str(), scaledFontSize);
        contentWidth = std::max(contentWidth, lineWidth);
        contentHeight += scaledFontSize + lineSpacing;
    }
    contentHeight -= lineSpacing;

    float boxWidth = std::max(static_cast<float>(contentWidth + _padding * 2), _minWidth);
    float boxHeight = static_cast<float>(contentHeight + _padding * 2);

    Vector2 pos = _resolvePosition(boxWidth, boxHeight);
    Rectangle rect = {pos.x, pos.y, boxWidth, boxHeight};
    _lastBounds = rect;

    Color bg = {_bgColor.r, _bgColor.g, _bgColor.b, _bgAlpha};
    DrawRectangleRounded(rect, 0.2f, 8, bg);
    if (_borderThickness > 0)
        DrawRectangleRoundedLines(rect, 0.2f, 8, static_cast<float>(_borderThickness),
                                  _borderColor);

    float textY = pos.y + _padding;
    for (const auto& line : _lines) {
        float textX = pos.x + _padding;
        for (size_t i = 0; i < line.segments.size(); ++i) {
            DrawText(line.segments[i].c_str(), static_cast<int>(textX), static_cast<int>(textY),
                     scaledFontSize, line.colors[i]);
            textX += MeasureText(line.segments[i].c_str(), scaledFontSize);
        }
        textY += scaledFontSize + lineSpacing;
    }
}
