#pragma once

#include "raylib.h"

#include <string>

class TextRenderer {
public:
    /**
     * @brief Draws centered text at a 3D world position.
     * @param worldPos 3D position in world space.
     * @param camera Camera for world→screen projection.
     * @param label Text to display.
     * @param fontSize Font size in pixels.
     * @param color Text color.
     */
    static void drawTextAt3DPosition(const Vector3& worldPos, const Camera3D& camera,
                                     const std::string& label, int fontSize, Color color);
};