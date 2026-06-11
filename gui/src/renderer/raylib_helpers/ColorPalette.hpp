#pragma once

#include "raylib.h"

/**
 * @brief Centralized color palette for teams and entity materials.
 */
class ColorPalette {
    public:
    struct SlimePalette {
        Color outer;  // mat[1] - outer shell. SKIP if {0,0,0,0}
        Color inner;  // mat[3] - inner body.  SKIP if {0,0,0,0}
        Color blush;  // mat[2] - accent.      SKIP if {0,0,0,0}
    };

    /** Sentinel: do not modify this material slot. */
    static constexpr Color KEEP = {0, 0, 0, 0};

    /**
     * @brief Returns a unique color for a team based on its index. Will repeat if more teams than
     * palette size.
     * @param index Index of the team (0-based).
     * @return Color for the team.
     */
    static Color getTeamColor(int index);

    /**
     * @brief Returns a slime palette with variations based on the team color.
     * @param teamColor The base color of the team.
     * @return SlimePalette with colors derived from the team color.
     */
    static SlimePalette getSlimePalette(Color teamColor);

    /**
     * @brief Utility to compare two colors for equality.
     * @param a First color.
     * @param b Second color.
     * @return True if colors are equal, false otherwise.
     */
    static bool colorEquals(Color a, Color b);

    static constexpr int PALETTE_SIZE = 10;

    private:
    static const Color _teamColors[PALETTE_SIZE];
    static const SlimePalette _slimePalettes[PALETTE_SIZE];
};
