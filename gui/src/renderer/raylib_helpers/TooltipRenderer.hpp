#pragma once

#include <string>
#include <vector>

#include "raylib.h"

/**
 * @brief Builder-pattern renderer for multiline tooltips with flexible styling.
 */
class TooltipRenderer {
    public:
    enum class Anchor {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        Center,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    class Builder {
        public:
        Builder();

        /**
         * @brief Adds a single-color line to the tooltip.
         * @param text Text content.
         * @param color Text color.
         * @return Reference to this builder.
         */
        Builder& addLine(const std::string& text, Color color = WHITE);

        /**
         * @brief Adds a line with multiple colored segments.
         * @param segments Vector of text segments.
         * @param colors Vector of colors (must match segments size).
         * @return Reference to this builder.
         * @throws std::invalid_argument if segments.size() != colors.size().
         */
        Builder& addColoredText(const std::vector<std::string>& segments,
                                const std::vector<Color>& colors);

        /**
         * @brief Sets the background color (ignoring alpha).
         * @param color Background color.
         * @return Reference to this builder.
         */
        Builder& setBackgroundColor(Color color);

        /**
         * @brief Sets the background alpha transparency.
         * @param alpha Alpha value (0-255).
         * @return Reference to this builder.
         */
        Builder& setBackgroundAlpha(unsigned char alpha);

        /**
         * @brief Sets the border color.
         * @param color Border color.
         * @return Reference to this builder.
         */
        Builder& setBorderColor(Color color);

        /**
         * @brief Sets the border thickness.
         * @param thickness Border thickness in pixels (0 = no border).
         * @return Reference to this builder.
         */
        Builder& setBorderThickness(int thickness);

        /**
         * @brief Sets the font size for all text.
         * @param size Font size in pixels.
         * @return Reference to this builder.
         */
        Builder& setFontSize(int size);

        /**
         * @brief Sets the internal padding.
         * @param padding Padding in pixels.
         * @return Reference to this builder.
         */
        Builder& setPadding(int padding);

        /**
         * @brief Sets the anchor point for positioning.
         * @param anchor Anchor position.
         * @return Reference to this builder.
         */
        Builder& setAnchor(Anchor anchor);

        /**
         * @brief Draws the tooltip at the given position.
         * @param position Screen position (anchor point).
         */
        void draw(Vector2 position);

        private:
        struct ColoredSegment {
            std::string text;
            Color color;
        };

        std::vector<std::vector<ColoredSegment>> _lines;
        Color _bgColor;
        unsigned char _bgAlpha;
        Color _borderColor;
        int _borderThickness;
        int _fontSize;
        int _padding;
        Anchor _anchor;

        Vector2 _calculateAnchoredPosition(Vector2 position, int boxWidth, int boxHeight) const;
    };

    /**
     * @brief Creates a new tooltip builder.
     * @return A new Builder instance.
     */
    static Builder create();
};
