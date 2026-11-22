#include "effects.hpp"

#include "painter.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kGravity = 900.0f;
constexpr float kPi = 3.14159265f;

template <typename T, typename Pred>
void eraseIf(std::vector<T>& items, Pred pred)
{
    items.erase(std::remove_if(items.begin(), items.end(), pred), items.end());
}

}  // namespace

Effects::Effects()
    : rng_(std::random_device{}())
{
}

float Effects::random(float lo, float hi)
{
    return std::uniform_real_distribution<float>(lo, hi)(rng_);
}

void Effects::burst(float x, float y, SDL_Color c, int count)
{
    for (int i = 0; i < count; ++i) {
        const float angle = random(0.0f, 2.0f * kPi);
        const float speed = random(60.0f, 260.0f);
        const float life = random(0.45f, 0.9f);
        particles_.push_back({
            x, y,
            std::cos(angle) * speed, std::sin(angle) * speed - 140.0f,
            life, life,
            random(3.0f, 7.0f),
            c,
        });
    }
}

void Effects::popup(std::string text, float x, float y, SDL_Color c, int scale)
{
    popups_.push_back({std::move(text), x, y, 1.1f, 1.1f, scale, c});
}

void Effects::bump(float strength)
{
    bump_ = std::max(bump_, strength);
}

void Effects::update(double dt)
{
    const float step = static_cast<float>(dt);

    for (Particle& p : particles_) {
        p.vy += kGravity * step;
        p.x += p.vx * step;
        p.y += p.vy * step;
        p.life -= step;
    }
    eraseIf(particles_, [](const Particle& p) { return p.life <= 0.0f; });

    for (Popup& p : popups_) {
        p.y -= 45.0f * step;
        p.life -= step;
    }
    eraseIf(popups_, [](const Popup& p) { return p.life <= 0.0f; });

    bump_ = std::max(0.0f, bump_ - bump_ * 14.0f * step - 4.0f * step);
}

void Effects::draw(Painter& painter) const
{
    for (const Particle& p : particles_) {
        const float t = p.life / p.maxLife;
        const int size = std::max(1, static_cast<int>(p.size * t + 1.0f));
        painter.fillRoundedRect(static_cast<int>(p.x) - size / 2, static_cast<int>(p.y) - size / 2,
                                size, size, size / 3,
                                color::withAlpha(p.color, static_cast<Uint8>(255 * t)));
    }

    for (const Popup& p : popups_) {
        const float t = p.life / p.maxLife;
        const Uint8 alpha = static_cast<Uint8>(255 * std::min(1.0f, t * 2.5f));
        painter.textCentered(p.text, static_cast<int>(p.x), static_cast<int>(p.y), p.scale,
                             color::withAlpha(p.color, alpha));
    }
}

void Effects::clear()
{
    particles_.clear();
    popups_.clear();
    bump_ = 0.0f;
}
