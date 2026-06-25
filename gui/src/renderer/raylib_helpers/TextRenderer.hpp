#pragma once

#include <string>

#include "raylib.h"

/**
 * @brief Central wrapper for all GUI text. Owns one shared Font (JetBrainsMono)
 * loaded once for the window lifetime; falls back to GetFontDefault() if unloaded.
 */
class TextRenderer {
    public:
    static bool loadFont(const std::string& path);  // needs GL context
    static void unloadFont();                       // needs GL context

    static void draw(const std::string& text, int x, int y, int fontSize, Color color);
    static int measure(const std::string& text, int fontSize);

    private:
    static Font _font;
    static bool _loaded;

    static constexpr int BASE_SIZE =
        32;  // glyph atlas bake size (near render size to limit downscale blur)
    static float _spacing(int fontSize);
    static const Font& _active();
};
