#pragma once

#include <SDL.h>

#include <random>
#include <string>
#include <vector>

class Painter;

class Effects {
public:
    Effects();

    void burst(float x, float y, SDL_Color c, int count);
    void popup(std::string text, float x, float y, SDL_Color c, int scale);
    void bump(float strength);

    void update(double dt);
    void draw(Painter& painter) const;
    void clear();

    int offsetY() const { return static_cast<int>(bump_); }

private:
    struct Particle {
        float x, y;
        float vx, vy;
        float life, maxLife;
        float size;
        SDL_Color color;
    };

    struct Popup {
        std::string text;
        float x, y;
        float life, maxLife;
        int scale;
        SDL_Color color;
    };

    float random(float lo, float hi);

    std::vector<Particle> particles_;
    std::vector<Popup> popups_;
    std::mt19937 rng_;
    float bump_ = 0.0f;
};
