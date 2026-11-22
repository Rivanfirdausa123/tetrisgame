#pragma once

#include "effects.hpp"
#include "game.hpp"

#include <SDL.h>

#include <memory>
#include <vector>

class Painter;

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int run();

private:
    enum class Screen { Title, Playing, Paused, GameOver };

    struct Floater {
        float x, y;
        float speed;
        float phase;
        int size;
        SDL_Color color;
    };

    void handleEvent(const SDL_Event& event);
    void handleKey(SDL_Keycode key);
    void update(double dt);
    void updateHorizontalInput(double dt);
    void processGameEvents();

    void startGame();
    void loadBest();
    void saveBest() const;

    void render();
    void drawBackground();
    void drawBoard();
    void drawHoldPanel();
    void drawControlsPanel();
    void drawNextPanel();
    void drawStatsPanel();
    void drawTitle();
    void drawPause();
    void drawGameOver();
    void drawPiecePreview(PieceType type, int centerX, int centerY, int cellSize, bool dimmed);
    void drawOverlayCard(int height);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    std::unique_ptr<Painter> painter_;

    Game game_;
    Effects effects_;
    Screen screen_ = Screen::Title;
    bool running_ = true;

    std::vector<Floater> floaters_;
    double clock_ = 0.0;
    double overTimer_ = 0.0;

    int moveDir_ = 0;
    double moveHeld_ = 0.0;
    double moveRepeat_ = 0.0;

    int best_ = 0;
    bool newBest_ = false;
};
