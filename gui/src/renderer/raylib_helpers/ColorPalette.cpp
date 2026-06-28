#include "ColorPalette.hpp"

const Color ColorPalette::_teamColors[PALETTE_SIZE] = {RED,    GREEN, BLUE, YELLOW,  ORANGE,
                                                       PURPLE, PINK,  LIME, SKYBLUE, MAGENTA};

// KEEP = {0,0,0,0} = don't touch that material slot
const ColorPalette::SlimePalette ColorPalette::_slimePalettes[PALETTE_SIZE] = {
    // RED        blush: orange (red+yellow)
    {RED, RED, {255, 140, 0, 255}},
    // GREEN      blush: cyan (green+blue)
    {GREEN, GREEN, {0, 200, 200, 255}},
    // BLUE       model as-is, purple blush already baked in
    {KEEP, KEEP, KEEP},
    // YELLOW     blush: orange (yellow+red)
    {YELLOW, YELLOW, {255, 140, 0, 255}},
    // ORANGE     blush: red (orange+red)
    {ORANGE, ORANGE, {200, 50, 50, 255}},
    // PURPLE     blush: magenta (purple+red)
    {PURPLE, PURPLE, {180, 0, 180, 255}},
    // PINK       blush: purple (pink+blue)
    {PINK, PINK, {160, 60, 200, 255}},
    // LIME       blush: green (lime+green)
    {LIME, LIME, {0, 180, 60, 255}},
    // SKYBLUE    blush: blue (skyblue+blue)
    {SKYBLUE, SKYBLUE, {60, 100, 220, 255}},
    // MAGENTA    blush: purple (magenta+blue)
    {MAGENTA, MAGENTA, {160, 0, 200, 255}},
};

Color ColorPalette::getTeamColor(int index) { return _teamColors[index % PALETTE_SIZE]; }

ColorPalette::SlimePalette ColorPalette::getSlimePalette(Color teamColor)
{
    for (int i = 0; i < PALETTE_SIZE; i++) {
        if (colorEquals(_teamColors[i], teamColor)) return _slimePalettes[i];
    }
    // fallback: raw tint on outer+inner, leave blush
    return {teamColor, teamColor, KEEP};
}

bool ColorPalette::colorEquals(Color a, Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

Color ColorPalette::getResourceColor(int resourceIndex)
{
    static const Color resourceColors[] = {
        RED,       // food
        DARKGRAY,  // linemate
        GREEN,     // deraumere
        BLUE,      // sibur
        YELLOW,    // mendiane
        ORANGE,    // phiras
        PURPLE     // thystame
    };

    if (resourceIndex < 0 || resourceIndex >= 7) return WHITE;
    return resourceColors[resourceIndex];
}
