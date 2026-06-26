#include "TextRenderer.hpp"

#include <vector>

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

// Codepoints to bake: ASCII (32–126) + Latin-1 Supplement (160–255).
// Covers é, è, ê, à, ù, ç, ô, î, û and all FR characters used in I18n strings.
static std::vector<int> _buildCodepoints()
{
    std::vector<int> cp;
    cp.reserve(95 + 96);
    for (int i = 32; i <= 126; ++i) cp.push_back(i);
    for (int i = 160; i <= 255; ++i) cp.push_back(i);
    return cp;
}

bool TextRenderer::loadFont(const std::string& path)
{
    if (_loaded) unloadFont();

    static auto codepoints = _buildCodepoints();
    _font =
        LoadFontEx(path.c_str(), BASE_SIZE, codepoints.data(), static_cast<int>(codepoints.size()));
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
