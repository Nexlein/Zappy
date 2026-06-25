/// @file IWidget.hpp
/// @brief Interface for GUI widgets.

#pragma once

#include <optional>

#include "SelectionFinder.hpp"

/// @brief Interface for GUI widgets.
class IWidget {
    public:
    virtual ~IWidget() = default;

    /// @brief Draws the widget.
    /// @param scaledFontSize Base font size for UI scaling.
    virtual void draw(int scaledFontSize) const = 0;

    /// @brief Processes user input.
    /// @return true if the widget consumed the input, false otherwise.
    virtual bool handleInput() = 0;
};
