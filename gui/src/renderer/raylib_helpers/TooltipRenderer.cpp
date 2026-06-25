#include "TooltipRenderer.hpp"

#include <stdexcept>

#include "TextRenderer.hpp"

TooltipRenderer::Builder TooltipRenderer::create() { return Builder(); }

TooltipRenderer::Builder::Builder()
    : _bgColor(BLACK),
      _bgAlpha(200),
      _borderColor(WHITE),
      _borderThickness(2),
      _fontSize(20),
      _padding(10),
      _anchor(Anchor::TopLeft)
{
}

TooltipRenderer::Builder& TooltipRenderer::Builder::addLine(const std::string& text, Color color)
{
    _lines.push_back({{text, color}});
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::addColoredText(
    const std::vector<std::string>& segments, const std::vector<Color>& colors)
{
    if (segments.size() != colors.size()) {
        throw std::invalid_argument("Segments and colors size mismatch");
    }

    std::vector<ColoredSegment> line;
    for (size_t i = 0; i < segments.size(); ++i) {
        line.push_back({segments[i], colors[i]});
    }
    _lines.push_back(line);
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setBackgroundColor(Color color)
{
    _bgColor = color;
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setBackgroundAlpha(unsigned char alpha)
{
    _bgAlpha = alpha;
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setBorderColor(Color color)
{
    _borderColor = color;
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setBorderThickness(int thickness)
{
    _borderThickness = thickness;
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setFontSize(int size)
{
    _fontSize = size;
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setPadding(int padding)
{
    _padding = padding;
    return *this;
}

TooltipRenderer::Builder& TooltipRenderer::Builder::setAnchor(Anchor anchor)
{
    _anchor = anchor;
    return *this;
}

void TooltipRenderer::Builder::draw(Vector2 position)
{
    if (_lines.empty()) return;

    int lineSpacing = _fontSize / 4;  // 25% of font size spacing between lines
    int boxWidth = 0;
    int boxHeight = 0;

    for (const auto& line : _lines) {
        int lineWidth = 0;
        for (const auto& segment : line) {
            lineWidth += TextRenderer::measure(segment.text, _fontSize);
        }
        boxWidth = std::max(boxWidth, lineWidth);
        boxHeight += _fontSize + lineSpacing;
    }
    boxHeight -= lineSpacing;  // Remove spacing after last line

    boxWidth += _padding * 2;
    boxHeight += _padding * 2;

    Vector2 anchoredPos = _calculateAnchoredPosition(position, boxWidth, boxHeight);
    Rectangle rect = {anchoredPos.x, anchoredPos.y, static_cast<float>(boxWidth),
                      static_cast<float>(boxHeight)};

    // Draw background
    Color bgColorWithAlpha = {_bgColor.r, _bgColor.g, _bgColor.b, _bgAlpha};
    DrawRectangleRounded(rect, 0.2f, 8, bgColorWithAlpha);

    // Draw border
    if (_borderThickness > 0) {
        DrawRectangleRoundedLines(rect, 0.2f, 8, _borderThickness, _borderColor);
    }

    // Draw text
    float textY = anchoredPos.y + _padding;
    for (const auto& line : _lines) {
        float textX = anchoredPos.x + _padding;
        for (const auto& segment : line) {
            TextRenderer::draw(segment.text, static_cast<int>(textX), static_cast<int>(textY),
                               _fontSize, segment.color);
            textX += TextRenderer::measure(segment.text, _fontSize);
        }
        textY += _fontSize + lineSpacing;
    }
}

Vector2 TooltipRenderer::Builder::_calculateAnchoredPosition(Vector2 position, int boxWidth,
                                                             int boxHeight) const
{
    Vector2 anchoredPos = position;

    switch (_anchor) {
        case Anchor::TopLeft:
            break;
        case Anchor::TopCenter:
            anchoredPos.x -= boxWidth / 2.0f;
            break;
        case Anchor::TopRight:
            anchoredPos.x -= boxWidth;
            break;
        case Anchor::MiddleLeft:
            anchoredPos.y -= boxHeight / 2.0f;
            break;
        case Anchor::Center:
            anchoredPos.x -= boxWidth / 2.0f;
            anchoredPos.y -= boxHeight / 2.0f;
            break;
        case Anchor::MiddleRight:
            anchoredPos.x -= boxWidth;
            anchoredPos.y -= boxHeight / 2.0f;
            break;
        case Anchor::BottomLeft:
            anchoredPos.y -= boxHeight;
            break;
        case Anchor::BottomCenter:
            anchoredPos.x -= boxWidth / 2.0f;
            anchoredPos.y -= boxHeight;
            break;
        case Anchor::BottomRight:
            anchoredPos.x -= boxWidth;
            anchoredPos.y -= boxHeight;
            break;
    }

    return anchoredPos;
}