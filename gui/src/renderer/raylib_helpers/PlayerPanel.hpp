/// @file PlayerPanel.hpp
/// @brief Widget displaying players and allowing selection.

#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "ButtonWidget.hpp"
#include "IWidget.hpp"
#include "SelectionFinder.hpp"
#include "TooltipWidget.hpp"
#include "core/WorldState.hpp"
#include "raylib.h"

/// @brief Widget displaying players and allowing selection.
class PlayerPanel : public IWidget {
    public:
    PlayerPanel() = default;
    ~PlayerPanel() override = default;

    /// @brief Sets the world state pointer.
    /// @param world Pointer to the current world state.
    void setWorld(const WorldState* world);

    /// @brief Draws the panel and player list.
    /// @param scaledFontSize Base font size for UI scaling.
    void draw(int scaledFontSize) const override;

    /// @brief Processes mouse clicks on player rows.
    /// @return true if the widget consumed the input.
    bool handleInput() override;

    /// @brief Gets and clears any pending selection made by the user.
    /// @return The pending selection, or nullopt if none.
    std::optional<SelectionFinder::Selection> getPendingSelection();

    /// @brief Checks if panel is currently open.
    /// @return true if open.
    bool isOpen() const { return _isOpen; }

    /// @brief Sets the team color resolution function.
    void setTeamColorFunc(std::function<Color(const std::string&)> func)
    {
        _colorFunc = std::move(func);
    }

    private:
    const WorldState* _world = nullptr;
    bool _isOpen = false;
    std::optional<SelectionFinder::Selection> _pendingSelection = std::nullopt;
    std::function<Color(const std::string&)> _colorFunc;

    mutable TooltipWidget _background;
    mutable std::vector<ButtonWidget> _playerButtons;

    static constexpr int PANEL_WIDTH = 400;
    static constexpr int ROW_HEIGHT = 36;
};
