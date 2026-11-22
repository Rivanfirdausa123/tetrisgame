#pragma once

#include <SDL.h>

#include <string_view>

namespace color {

SDL_Color shade(SDL_Color c, float factor);
SDL_Color mix(SDL_Color a, SDL_Color b, float t);
SDL_Color withAlpha(SDL_Color c, Uint8 alpha);

}  // namespace color

class Painter {
public:
    explicit Painter(SDL_Renderer* renderer);

    void clear(SDL_Color c);
    void fillRect(int x, int y, int w, int h, SDL_Color c);
    void fillRoundedRect(int x, int y, int w, int h, int radius, SDL_Color c);
    void panel(int x, int y, int w, int h, int radius, SDL_Color fill, SDL_Color border);
    void verticalGradient(int x, int y, int w, int h, SDL_Color top, SDL_Color bottom);

    void block(int x, int y, int size, SDL_Color c);
    void ghostBlock(int x, int y, int size, SDL_Color c, SDL_Color background);

    void text(std::string_view s, int x, int y, int scale, SDL_Color c);
    void textShadow(std::string_view s, int x, int y, int scale, SDL_Color c);
    void textCentered(std::string_view s, int centerX, int y, int scale, SDL_Color c);
    void textRight(std::string_view s, int rightX, int y, int scale, SDL_Color c);

    static int textWidth(std::string_view s, int scale);

private:
    void setColor(SDL_Color c);

    SDL_Renderer* renderer_;
};
