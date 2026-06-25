#include "TextRenderer.hpp"

Font TextRenderer::_font = {};
bool TextRenderer::_loaded = false;

float TextRenderer::_spacing(int)
{
    return 0.0f;  // monospace advance is baked into glyphs; no extra gap
}

const Font& TextRenderer::_active()
{
    static const Font fallback = GetFontDefault();
    return _loaded ? _font : fallback;
}

bool TextRenderer::loadFont(const std::string& path)
{
    if (_loaded) unloadFont();

    _font = LoadFontEx(path.c_str(), BASE_SIZE, nullptr, 0);
    if (_font.texture.id == 0) return false;

    SetTextureFilter(_font.texture, TEXTURE_FILTER_BILINEAR);  // smooth when scaled
    _loaded = true;
    return true;
}

void TextRenderer::unloadFont()
{
    if (!_loaded) return;
    UnloadFont(_font);
    _font = {};
    _loaded = false;
}

void TextRenderer::draw(const std::string& text, int x, int y, int fontSize, Color color)
{
    Vector2 pos = {static_cast<float>(x), static_cast<float>(y)};
    DrawTextEx(_active(), text.c_str(), pos, static_cast<float>(fontSize), _spacing(fontSize),
               color);
}

int TextRenderer::measure(const std::string& text, int fontSize)
{
    return static_cast<int>(
        MeasureTextEx(_active(), text.c_str(), static_cast<float>(fontSize), _spacing(fontSize)).x);
}
