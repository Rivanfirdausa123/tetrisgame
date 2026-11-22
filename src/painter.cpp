#include "painter.hpp"

#include "font.hpp"

#include <algorithm>
#include <cmath>

namespace color {

namespace {

Uint8 clampByte(float v)
{
    return static_cast<Uint8>(std::clamp(v, 0.0f, 255.0f));
}

}  // namespace

SDL_Color shade(SDL_Color c, float factor)
{
    return {clampByte(c.r * factor), clampByte(c.g * factor), clampByte(c.b * factor), c.a};
}

SDL_Color mix(SDL_Color a, SDL_Color b, float t)
{
    return {
        clampByte(a.r + (b.r - a.r) * t),
        clampByte(a.g + (b.g - a.g) * t),
        clampByte(a.b + (b.b - a.b) * t),
        clampByte(a.a + (b.a - a.a) * t),
    };
}

SDL_Color withAlpha(SDL_Color c, Uint8 alpha)
{
    c.a = alpha;
    return c;
}

}  // namespace color

Painter::Painter(SDL_Renderer* renderer)
    : renderer_(renderer)
{
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void Painter::setColor(SDL_Color c)
{
    SDL_SetRenderDrawColor(renderer_, c.r, c.g, c.b, c.a);
}

void Painter::clear(SDL_Color c)
{
    setColor(c);
    SDL_RenderClear(renderer_);
}

void Painter::fillRect(int x, int y, int w, int h, SDL_Color c)
{
    if (w <= 0 || h <= 0) {
        return;
    }
    setColor(c);
    const SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer_, &rect);
}

void Painter::fillRoundedRect(int x, int y, int w, int h, int radius, SDL_Color c)
{
    radius = std::min({radius, w / 2, h / 2});
    if (radius <= 0) {
        fillRect(x, y, w, h, c);
        return;
    }

    setColor(c);
    const SDL_Rect middle{x, y + radius, w, h - radius * 2};
    SDL_RenderFillRect(renderer_, &middle);

    for (int i = 0; i < radius; ++i) {
        const float dy = radius - i - 0.5f;
        const int inset = radius - static_cast<int>(std::lround(std::sqrt(radius * radius - dy * dy)));
        const SDL_Rect top{x + inset, y + i, w - inset * 2, 1};
        const SDL_Rect bottom{x + inset, y + h - 1 - i, w - inset * 2, 1};
        SDL_RenderFillRect(renderer_, &top);
        SDL_RenderFillRect(renderer_, &bottom);
    }
}

void Painter::panel(int x, int y, int w, int h, int radius, SDL_Color fill, SDL_Color border)
{
    fillRoundedRect(x, y, w, h, radius, border);
    fillRoundedRect(x + 2, y + 2, w - 4, h - 4, radius - 2, fill);
}

void Painter::verticalGradient(int x, int y, int w, int h, SDL_Color top, SDL_Color bottom)
{
    for (int row = 0; row < h; ++row) {
        setColor(color::mix(top, bottom, static_cast<float>(row) / std::max(1, h - 1)));
        SDL_RenderDrawLine(renderer_, x, y + row, x + w - 1, y + row);
    }
}

void Painter::block(int x, int y, int size, SDL_Color c)
{
    const int radius = std::max(2, size / 6);
    const int depth = std::max(2, size / 10);
    const SDL_Color white{255, 255, 255, c.a};

    fillRoundedRect(x + 1, y + 1, size - 2, size - 2, radius, color::shade(c, 0.68f));
    fillRoundedRect(x + 1, y + 1, size - 2, size - 2 - depth, radius, c);
    fillRoundedRect(x + size / 5, y + size / 7, size / 2, std::max(2, size / 9), 2,
                    color::withAlpha(color::mix(c, white, 0.55f), static_cast<Uint8>(c.a * 0.85f)));
}

void Painter::ghostBlock(int x, int y, int size, SDL_Color c, SDL_Color background)
{
    const int radius = std::max(2, size / 6);
    fillRoundedRect(x + 2, y + 2, size - 4, size - 4, radius, color::withAlpha(c, 150));
    fillRoundedRect(x + 4, y + 4, size - 8, size - 8, radius - 1, background);
    fillRoundedRect(x + 4, y + 4, size - 8, size - 8, radius - 1, color::withAlpha(c, 35));
}

void Painter::text(std::string_view s, int x, int y, int scale, SDL_Color c)
{
    setColor(c);
    int cursor = x;
    for (char ch : s) {
        if (const font::Glyph* g = font::glyph(ch)) {
            for (int row = 0; row < font::kGlyphHeight; ++row) {
                const std::uint8_t bits = (*g)[row];
                for (int col = 0; col < font::kGlyphWidth; ++col) {
                    if (bits & (1u << (font::kGlyphWidth - 1 - col))) {
                        const SDL_Rect px{cursor + col * scale, y + row * scale, scale, scale};
                        SDL_RenderFillRect(renderer_, &px);
                    }
                }
            }
        }
        cursor += font::kAdvance * scale;
    }
}

void Painter::textShadow(std::string_view s, int x, int y, int scale, SDL_Color c)
{
    const int offset = std::max(1, scale / 2);
    text(s, x + offset, y + offset, scale, SDL_Color{12, 10, 24, static_cast<Uint8>(c.a * 0.6f)});
    text(s, x, y, scale, c);
}

void Painter::textCentered(std::string_view s, int centerX, int y, int scale, SDL_Color c)
{
    textShadow(s, centerX - textWidth(s, scale) / 2, y, scale, c);
}

void Painter::textRight(std::string_view s, int rightX, int y, int scale, SDL_Color c)
{
    text(s, rightX - textWidth(s, scale), y, scale, c);
}

int Painter::textWidth(std::string_view s, int scale)
{
    if (s.empty()) {
        return 0;
    }
    return static_cast<int>(s.size()) * font::kAdvance * scale - scale;
}
