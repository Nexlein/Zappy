#pragma once

#include <cstdint>
#include <string>

#include "raylib.h"

/**
 * @brief Semi-transparent end-game overlay.
 *
 * Displays the winning team name, the game duration, and a Quit button.
 * Matches the HUD visual style (same rounded panel, colors, font scaling).
 * Call draw() every frame once the game has ended; it returns true when the
 * user clicks the Quit button.
 */
class WinScreen {
    public:
    /**
     * @brief Draws the overlay.
     * @param winnerTeam   Name of the winning team.
     * @param winnerColor  Team color used to highlight the team name.
     * @param uptimeSeconds Total elapsed server uptime at game end.
     * @param scaledFontSize Pre-scaled font size matching the HUD (use _getScaledFontSize(18)).
     * @return true if the Quit button was clicked this frame.
     */
    /**
     * @param teamJoinSeconds Seconds from gtt response (when winning team first joined).
     * @param serverUptimeSeconds Current server uptime from stu (game end time).
     * Duration shown = serverUptimeSeconds - teamJoinSeconds.
     */
    static bool draw(const std::string& winnerTeam, Color winnerColor, int teamJoinSeconds,
                     int64_t teamJoinTicks, unsigned int serverUptimeSeconds, int scaledFontSize);

    private:
    static std::string _formatUptime(unsigned int seconds);
    static bool _drawButton(const char* label, Rectangle rect, int fontSize);
};
