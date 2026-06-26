/// @file WinScreen.hpp
/// @brief End-game overlay widget.

#pragma once

#include <cstdint>
#include <string>

#include "ButtonWidget.hpp"
#include "IWidget.hpp"
#include "TooltipWidget.hpp"
#include "raylib.h"

/// @brief Semi-transparent overlay shown when a team wins.
///
/// Composed of a TooltipWidget (title + team name + duration) and a
/// ButtonWidget (Quit). Both components are owned and driven internally.
/// Call draw() every frame once the game has ended; poll quitRequested()
/// to know when the player clicked Quit.
///
/// Usage:
///   winScreen.setWinner(teamName, teamColor);
///   winScreen.setDuration(durationSeconds, durationTicks);
///   if (winScreen.handleInput()) { ... }   // optional: blocks sibling input
///   winScreen.draw(fontSize);
///   if (winScreen.quitRequested()) closeWindow();
class WinScreen : public IWidget {
    public:
    WinScreen();
    ~WinScreen() override = default;

    // ── Lifecycle ──────────────────────────────────────────────────────────
    void reset() { _quitRequested = false; }

    // ── Data setters ───────────────────────────────────────────────────────
    /// @brief Sets the winning team name and highlight color.
    void setWinner(const std::string& team, Color color);

    /// @brief Sets the win duration (server-authoritative, from gwt).
    /// @param seconds  Time the winner took to win, in seconds (-1 = not yet received).
    /// @param ticks    Same in game-time ticks.
    void setDuration(int seconds, int64_t ticks);

    // ── IWidget ────────────────────────────────────────────────────────────
    void draw(int scaledFontSize) const override;

    /// @brief Handles button input. Always returns true (overlay blocks all input).
    bool handleInput() override;

    // ── State polling ──────────────────────────────────────────────────────
    /// @brief Returns true once the Quit button has been clicked (latched — stays true).
    bool quitRequested() const { return _quitRequested; }

    private:
    std::string _winnerTeam;
    Color _winnerColor = WHITE;
    int _durationSeconds = -1;
    int64_t _durationTicks = -1;

    bool _quitRequested = false;

    mutable TooltipWidget _panel;
    mutable ButtonWidget _quitBtn;

    static std::string _formatDuration(unsigned int seconds);
    void _rebuildPanel(int scaledFontSize) const;
};
