/// @file ButtonWidget.cpp

#include "ButtonWidget.hpp"

ButtonWidget& ButtonWidget::setLabel(const std::string& label)
{
    _label = label;
    return *this;
}

ButtonWidget& ButtonWidget::setPosition(float x, float y)
{
    _x = x;
    _y = y;
    _anchor = Anchor::None;
    return *this;
}

ButtonWidget& ButtonWidget::setSize(float width, float height)
{
    _width = width;
    _height = height;
    return *this;
}

ButtonWidget& ButtonWidget::setAnchor(Anchor anchor, float margin)
{
    _anchor = anchor;
    _margin = margin;
    return *this;
}

ButtonWidget& ButtonWidget::setNormalColor(Color color)
{
    _normalColor = color;
    return *this;
}

ButtonWidget& ButtonWidget::setHoverColor(Color color)
{
    _hoverColor = color;
    return *this;
}

ButtonWidget& ButtonWidget::setTextColor(Color color)
{
    _textColor = color;
    return *this;
}

ButtonWidget& ButtonWidget::setBorderColor(Color color)
{
    _borderColor = color;
    return *this;
}

ButtonWidget& ButtonWidget::setBorderThickness(float thickness)
{
    _borderThickness = thickness;
    return *this;
}

ButtonWidget& ButtonWidget::setRoundness(float roundness)
{
    _roundness = roundness;
    return *this;
}

ButtonWidget& ButtonWidget::setOnClick(std::function<void()> callback)
{
    _onClick = std::move(callback);
    return *this;
}

Rectangle ButtonWidget::_resolvedBounds() const
{
    if (_anchor == Anchor::None) return {_x, _y, _width, _height};

    float sw = static_cast<float>(GetScreenWidth());
    float sh = static_cast<float>(GetScreenHeight());
    float x = 0.0f;
    float y = 0.0f;

    switch (_anchor) {
        case Anchor::TopLeft:
            x = _margin;
            y = _margin;
            break;
        case Anchor::TopCenter:
            x = (sw - _width) / 2.0f;
            y = _margin;
            break;
        case Anchor::TopRight:
            x = sw - _width - _margin;
            y = _margin;
            break;
        case Anchor::MiddleLeft:
            x = _margin;
            y = (sh - _height) / 2.0f;
            break;
        case Anchor::Center:
            x = (sw - _width) / 2.0f;
            y = (sh - _height) / 2.0f;
            break;
        case Anchor::MiddleRight:
            x = sw - _width - _margin;
            y = (sh - _height) / 2.0f;
            break;
        case Anchor::BottomLeft:
            x = _margin;
            y = sh - _height - _margin;
            break;
        case Anchor::BottomCenter:
            x = (sw - _width) / 2.0f;
            y = sh - _height - _margin;
            break;
        case Anchor::BottomRight:
            x = sw - _width - _margin;
            y = sh - _height - _margin;
            break;
        default:
            break;
    }

    return {x, y, _width, _height};
}

Rectangle ButtonWidget::getBounds() const { return _resolvedBounds(); }

bool ButtonWidget::handleInput()
{
    Rectangle bounds = _resolvedBounds();
    Vector2 mouse = GetMousePosition();
    _hovered = CheckCollisionPointRec(mouse, bounds);

    if (_hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        _clicked = true;
        if (_onClick) _onClick();
    }

    return _hovered;
}

bool ButtonWidget::wasClicked()
{
    bool result = _clicked;
    _clicked = false;
    return result;
}

void ButtonWidget::draw(int scaledFontSize) const
{
    Rectangle bounds = _resolvedBounds();

    DrawRectangleRounded(bounds, _roundness, 8, _hovered ? _hoverColor : _normalColor);
    if (_borderThickness > 0.0f)
        DrawRectangleRoundedLines(bounds, _roundness, 8, _borderThickness, _borderColor);

    if (!_label.empty()) {
        int tw = MeasureText(_label.c_str(), scaledFontSize);
        int tx = static_cast<int>(bounds.x + (bounds.width - tw) / 2.0f);
        int ty = static_cast<int>(bounds.y + (bounds.height - scaledFontSize) / 2.0f);
        DrawText(_label.c_str(), tx, ty, scaledFontSize, _textColor);
    }
}
