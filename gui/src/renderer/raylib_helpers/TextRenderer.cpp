#include "TextRenderer.hpp"

void TextRenderer::drawTextAt3DPosition(const Vector3& worldPos, const Camera3D& camera,
                                        const std::string& label, int fontSize, Color color)
{
    Vector2 screenPos = GetWorldToScreen(worldPos, camera);
    int textWidth = MeasureText(label.c_str(), fontSize);
    DrawText(label.c_str(), static_cast<int>(screenPos.x) - textWidth / 2,
             static_cast<int>(screenPos.y) - fontSize / 2, fontSize, color);
}