#include "app.hpp"

#include "painter.hpp"
#include "theme.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

namespace {

constexpr int kScreenWidth = 800;
constexpr int kScreenHeight = 720;

constexpr int kCell = 30;
constexpr int kVisibleRows = Game::kHeight - Game::kHiddenRows;
constexpr int kBoardWidth = Game::kWidth * kCell;
constexpr int kBoardHeight = kVisibleRows * kCell;
constexpr int kBoardX = (kScreenWidth - kBoardWidth) / 2;
constexpr int kBoardY = (kScreenHeight - kBoardHeight) / 2;

constexpr int kSideWidth = 180;
constexpr int kLeftX = kBoardX - 30 - kSideWidth;
constexpr int kRightX = kBoardX + kBoardWidth + 30;

constexpr double kAutoShiftDelay = 0.16;
constexpr double kAutoShiftRate = 0.045;

constexpr const char* kTitle = "BLOCKFALL";

std::string bestScorePath()
{
    std::string path = "best_score.txt";
    if (char* dir = SDL_GetPrefPath("blockfall", "blockfall")) {
        path = std::string(dir) + path;
        SDL_free(dir);
    }
    return path;
}

bool blink(double clock, double period)
{
    return std::fmod(clock, period) < period * 0.65;
}

}  // namespace

App::App()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    window_ = SDL_CreateWindow(kTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               kScreenWidth, kScreenHeight,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) {
        throw std::runtime_error(SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_RenderSetLogicalSize(renderer_, kScreenWidth, kScreenHeight);
    painter_ = std::make_unique<Painter>(renderer_);

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int i = 0; i < 16; ++i) {
        SDL_Color c = theme::kPieces[i % kPieceTypeCount];
        c.a = 55;
        floaters_.push_back({
            unit(rng) * kScreenWidth,
            unit(rng) * kScreenHeight,
            8.0f + unit(rng) * 22.0f,
            unit(rng) * 6.28f,
            18 + static_cast<int>(unit(rng) * 30.0f),
            c,
        });
    }

    loadBest();
}

App::~App()
{
    painter_.reset();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

int App::run()
{
    Uint64 previous = SDL_GetPerformanceCounter();
    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        const double dt = std::min(0.05, (now - previous) / frequency);
        previous = now;

        update(dt);
        render();
    }
    return 0;
}

void App::handleEvent(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_QUIT:
        running_ = false;
        break;
    case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST && screen_ == Screen::Playing) {
            screen_ = Screen::Paused;
        }
        break;
    case SDL_KEYDOWN:
        if (!event.key.repeat) {
            handleKey(event.key.keysym.sym);
        }
        break;
    default:
        break;
    }
}

void App::handleKey(SDL_Keycode key)
{
    switch (screen_) {
    case Screen::Title:
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) {
            startGame();
        } else if (key == SDLK_ESCAPE) {
            running_ = false;
        }
        break;

    case Screen::Playing:
        switch (key) {
        case SDLK_LEFT:
        case SDLK_RIGHT:
            moveDir_ = key == SDLK_LEFT ? -1 : 1;
            moveHeld_ = 0.0;
            moveRepeat_ = 0.0;
            game_.shift(moveDir_);
            break;
        case SDLK_UP:
        case SDLK_x:
            game_.rotate(1);
            break;
        case SDLK_z:
        case SDLK_LCTRL:
            game_.rotate(-1);
            break;
        case SDLK_SPACE:
            game_.hardDrop();
            break;
        case SDLK_c:
        case SDLK_LSHIFT:
            game_.hold();
            break;
        case SDLK_p:
        case SDLK_ESCAPE:
            screen_ = Screen::Paused;
            break;
        case SDLK_r:
            startGame();
            break;
        default:
            break;
        }
        break;

    case Screen::Paused:
        if (key == SDLK_p || key == SDLK_ESCAPE || key == SDLK_RETURN) {
            screen_ = Screen::Playing;
        } else if (key == SDLK_r) {
            startGame();
        }
        break;

    case Screen::GameOver:
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_r) {
            startGame();
        } else if (key == SDLK_ESCAPE) {
            screen_ = Screen::Title;
            game_.reset();
            effects_.clear();
        }
        break;
    }
}

void App::startGame()
{
    game_.reset();
    effects_.clear();
    screen_ = Screen::Playing;
    moveDir_ = 0;
    overTimer_ = 0.0;
    newBest_ = false;
}

void App::update(double dt)
{
    clock_ += dt;

    for (Floater& f : floaters_) {
        f.y -= f.speed * static_cast<float>(dt);
        f.x += std::sin(static_cast<float>(clock_) * 0.6f + f.phase) * 0.15f;
        if (f.y < -f.size) {
            f.y = static_cast<float>(kScreenHeight + f.size);
        }
    }

    effects_.update(dt);

    if (screen_ == Screen::GameOver) {
        overTimer_ += dt;
    }
    if (screen_ != Screen::Playing) {
        return;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    game_.setSoftDrop(keys[SDL_SCANCODE_DOWN] != 0);
    updateHorizontalInput(dt);

    game_.update(dt);
    processGameEvents();
}

void App::updateHorizontalInput(double dt)
{
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    const bool left = keys[SDL_SCANCODE_LEFT] != 0;
    const bool right = keys[SDL_SCANCODE_RIGHT] != 0;

    const bool released = (moveDir_ < 0 && !left) || (moveDir_ > 0 && !right);
    if (released) {
        moveDir_ = left ? -1 : (right ? 1 : 0);
        moveHeld_ = 0.0;
        moveRepeat_ = 0.0;
    }
    if (moveDir_ == 0) {
        return;
    }

    moveHeld_ += dt;
    if (moveHeld_ < kAutoShiftDelay) {
        return;
    }

    moveRepeat_ += dt;
    while (moveRepeat_ >= kAutoShiftRate) {
        moveRepeat_ -= kAutoShiftRate;
        if (!game_.shift(moveDir_)) {
            moveRepeat_ = 0.0;
            break;
        }
    }
}

void App::processGameEvents()
{
    static constexpr std::array<const char*, 5> kClearNames{"", "SINGLE", "DOUBLE", "TRIPLE", "QUAD!"};
    const float centerX = kBoardX + kBoardWidth / 2.0f;

    for (const Game::Event& event : game_.takeEvents()) {
        switch (event.type) {
        case Game::EventType::HardDrop:
            effects_.bump(std::min(10.0f, 3.0f + event.value * 0.4f));
            break;

        case Game::EventType::LinesCleared: {
            float rowSum = 0.0f;
            for (int row : game_.clearingRows()) {
                const float y = kBoardY + (row - Game::kHiddenRows + 0.5f) * kCell;
                rowSum += y;
                for (int x = 0; x < Game::kWidth; ++x) {
                    const int value = game_.cell(x, row);
                    if (value > 0) {
                        effects_.burst(kBoardX + (x + 0.5f) * kCell, y,
                                       theme::piece(static_cast<PieceType>(value - 1)), 3);
                    }
                }
            }
            const float popupY = rowSum / game_.clearingRows().size() - 20.0f;
            const bool quad = event.value >= 4;
            effects_.popup(kClearNames[std::min(event.value, 4)], centerX, popupY,
                           quad ? theme::kGold : theme::kText, quad ? 5 : 4);
            if (game_.combo() > 0) {
                effects_.popup("COMBO X" + std::to_string(game_.combo() + 1), centerX, popupY + 44.0f,
                               theme::kAccent, 2);
            }
            break;
        }

        case Game::EventType::LevelUp:
            effects_.popup("LEVEL " + std::to_string(event.value), centerX,
                           kBoardY + kBoardHeight / 3.0f, theme::kAccent, 4);
            break;

        case Game::EventType::GameOver:
            screen_ = Screen::GameOver;
            overTimer_ = 0.0;
            if (game_.score() > best_) {
                best_ = game_.score();
                newBest_ = true;
                saveBest();
            }
            break;

        case Game::EventType::Locked:
            break;
        }
    }
}

void App::loadBest()
{
    std::ifstream in(bestScorePath());
    if (!(in >> best_) || best_ < 0) {
        best_ = 0;
    }
}

void App::saveBest() const
{
    std::ofstream out(bestScorePath(), std::ios::trunc);
    out << best_ << '\n';
}

void App::render()
{
    drawBackground();
    drawHoldPanel();
    drawControlsPanel();
    drawNextPanel();
    drawStatsPanel();
    drawBoard();
    effects_.draw(*painter_);

    switch (screen_) {
    case Screen::Title: drawTitle(); break;
    case Screen::Paused: drawPause(); break;
    case Screen::GameOver: drawGameOver(); break;
    case Screen::Playing: break;
    }

    SDL_RenderPresent(renderer_);
}

void App::drawBackground()
{
    painter_->verticalGradient(0, 0, kScreenWidth, kScreenHeight, theme::kSkyTop, theme::kSkyBottom);
    for (const Floater& f : floaters_) {
        painter_->block(static_cast<int>(f.x), static_cast<int>(f.y), f.size, f.color);
    }
}

void App::drawBoard()
{
    const int bump = effects_.offsetY();
    const int originY = kBoardY + bump;

    painter_->panel(kBoardX - 12, originY - 12, kBoardWidth + 24, kBoardHeight + 24, 16,
                    theme::kBoard, theme::kPanelBorder);
    painter_->fillRect(kBoardX, originY, kBoardWidth, kBoardHeight, theme::kBoard);

    for (int x = 1; x < Game::kWidth; ++x) {
        painter_->fillRect(kBoardX + x * kCell, originY, 1, kBoardHeight, theme::kGrid);
    }
    for (int y = 1; y < kVisibleRows; ++y) {
        painter_->fillRect(kBoardX, originY + y * kCell, kBoardWidth, 1, theme::kGrid);
    }

    const auto cellX = [](int x) { return kBoardX + x * kCell; };
    const auto cellY = [originY](int y) { return originY + (y - Game::kHiddenRows) * kCell; };

    const std::vector<int>& clearing = game_.clearingRows();
    const float clearT = static_cast<float>(game_.clearProgress());
    const SDL_Color white{255, 255, 255, 255};
    const int grayLine = Game::kHeight - static_cast<int>(overTimer_ * 40.0);

    for (int y = Game::kHiddenRows; y < Game::kHeight; ++y) {
        const bool isClearing = std::find(clearing.begin(), clearing.end(), y) != clearing.end();
        for (int x = 0; x < Game::kWidth; ++x) {
            const int value = game_.cell(x, y);
            if (value == 0) {
                continue;
            }
            SDL_Color c = theme::piece(static_cast<PieceType>(value - 1));
            if (isClearing) {
                c = clearT < 0.5f ? color::mix(c, white, clearT * 2.0f)
                                  : color::withAlpha(white, static_cast<Uint8>(255 * (1.0f - clearT) * 2.0f));
            } else if (screen_ == Screen::GameOver && y >= grayLine) {
                c = theme::kStone;
            }
            painter_->block(cellX(x), cellY(y), kCell, c);
        }
    }

    const bool showPiece = (screen_ == Screen::Playing || screen_ == Screen::Paused)
                           && game_.state() == Game::State::Playing;
    if (!showPiece) {
        return;
    }

    const Game::Piece& piece = game_.current();
    const SDL_Color c = theme::piece(piece.type);
    const PieceCells& cells = pieceCells(piece.type, piece.rotation);
    const int ghost = game_.ghostY();

    if (ghost != piece.y) {
        for (const Offset& o : cells) {
            if (ghost + o.y >= Game::kHiddenRows) {
                painter_->ghostBlock(cellX(piece.x + o.x), cellY(ghost + o.y), kCell, c, theme::kBoard);
            }
        }
    }
    for (const Offset& o : cells) {
        if (piece.y + o.y >= Game::kHiddenRows) {
            painter_->block(cellX(piece.x + o.x), cellY(piece.y + o.y), kCell, c);
        }
    }
}

void App::drawPiecePreview(PieceType type, int centerX, int centerY, int cellSize, bool dimmed)
{
    const PieceCells& cells = pieceCells(type, 0);
    int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    for (const Offset& o : cells) {
        minX = std::min(minX, o.x);
        maxX = std::max(maxX, o.x);
        minY = std::min(minY, o.y);
        maxY = std::max(maxY, o.y);
    }

    const int originX = centerX - (maxX - minX + 1) * cellSize / 2 - minX * cellSize;
    const int originY = centerY - (maxY - minY + 1) * cellSize / 2 - minY * cellSize;

    SDL_Color c = theme::piece(type);
    if (dimmed) {
        c = color::mix(c, theme::kStone, 0.7f);
    }
    for (const Offset& o : cells) {
        painter_->block(originX + o.x * cellSize, originY + o.y * cellSize, cellSize, c);
    }
}

void App::drawHoldPanel()
{
    const int x = kLeftX;
    const int y = kBoardY;
    painter_->panel(x, y, kSideWidth, 150, 14, theme::kPanel, theme::kPanelBorder);
    painter_->text("HOLD", x + 18, y + 18, 2, theme::kTextDim);

    if (const auto held = game_.heldPiece(); held && screen_ != Screen::Title) {
        drawPiecePreview(*held, x + kSideWidth / 2, y + 90, 24, !game_.canHold());
    }
}

void App::drawControlsPanel()
{
    struct Binding {
        const char* action;
        const char* keys;
    };
    static constexpr std::array<Binding, 7> kBindings{{
        {"MOVE", "< >"},
        {"ROTATE", "UP X Z"},
        {"SOFT DROP", "DOWN"},
        {"HARD DROP", "SPACE"},
        {"HOLD", "C"},
        {"PAUSE", "P"},
        {"RESTART", "R"},
    }};

    const int x = kLeftX;
    const int y = kBoardY + 170;
    const int height = kBoardHeight - 170;
    painter_->panel(x, y, kSideWidth, height, 14, theme::kPanel, theme::kPanelBorder);
    painter_->text("CONTROLS", x + 18, y + 18, 2, theme::kTextDim);

    int rowY = y + 52;
    for (const Binding& b : kBindings) {
        painter_->text(b.action, x + 18, rowY, 2, theme::kText);
        const int capWidth = Painter::textWidth(b.keys, 2) + 14;
        painter_->fillRoundedRect(x + 18, rowY + 20, capWidth, 22, 6, color::shade(theme::kPanelBorder, 0.75f));
        painter_->text(b.keys, x + 25, rowY + 24, 2, theme::kAccent);
        rowY += 52;
    }
}

void App::drawNextPanel()
{
    const int x = kRightX;
    const int y = kBoardY;
    painter_->panel(x, y, kSideWidth, 290, 14, theme::kPanel, theme::kPanelBorder);
    painter_->text("NEXT", x + 18, y + 18, 2, theme::kTextDim);

    if (screen_ == Screen::Title) {
        return;
    }

    static constexpr std::array<int, Game::kPreviewCount> kOffsets{95, 175, 245};
    static constexpr std::array<int, Game::kPreviewCount> kSizes{24, 20, 20};
    for (int i = 0; i < Game::kPreviewCount; ++i) {
        drawPiecePreview(game_.preview(i), x + kSideWidth / 2, y + kOffsets[i], kSizes[i], false);
    }
}

void App::drawStatsPanel()
{
    const int x = kRightX;
    const int y = kBoardY + 310;
    painter_->panel(x, y, kSideWidth, kBoardHeight - 310, 14, theme::kPanel, theme::kPanelBorder);

    struct Stat {
        const char* label;
        int value;
        SDL_Color color;
    };
    const std::array<Stat, 4> stats{{
        {"SCORE", game_.score(), theme::kText},
        {"LEVEL", game_.level(), theme::kText},
        {"LINES", game_.lines(), theme::kText},
        {"BEST", std::max(best_, game_.score()), theme::kGold},
    }};

    int rowY = y + 20;
    for (const Stat& stat : stats) {
        painter_->text(stat.label, x + 18, rowY, 2, theme::kTextDim);
        painter_->textShadow(std::to_string(stat.value), x + 18, rowY + 22, 3, stat.color);
        rowY += 68;
    }
}

void App::drawOverlayCard(int height)
{
    painter_->fillRect(0, 0, kScreenWidth, kScreenHeight, theme::kOverlay);
    const int width = 400;
    painter_->panel((kScreenWidth - width) / 2, (kScreenHeight - height) / 2, width, height, 18,
                    theme::kPanel, theme::kPanelBorder);
}

void App::drawTitle()
{
    painter_->fillRect(0, 0, kScreenWidth, kScreenHeight, theme::kOverlay);

    const std::string title = kTitle;
    const int scale = 9;
    const int advance = 6 * scale;
    int letterX = (kScreenWidth - Painter::textWidth(title, scale)) / 2;
    for (std::size_t i = 0; i < title.size(); ++i) {
        const int bob = static_cast<int>(std::sin(clock_ * 3.0 + i * 0.55) * 6.0);
        const SDL_Color c = theme::kPieces[i % kPieceTypeCount];
        painter_->textShadow(std::string(1, title[i]), letterX, 200 + bob, scale, c);
        letterX += advance;
    }

    painter_->textCentered("A COZY FALLING BLOCK PUZZLE", kScreenWidth / 2, 290, 2, theme::kTextDim);

    const int blockSize = 26;
    const int rowX = (kScreenWidth - kPieceTypeCount * (blockSize + 8)) / 2;
    for (int i = 0; i < kPieceTypeCount; ++i) {
        const int lift = static_cast<int>(std::max(0.0, std::sin(clock_ * 4.0 - i * 0.7)) * 8.0);
        painter_->block(rowX + i * (blockSize + 8), 340 - lift, blockSize, theme::kPieces[i]);
    }

    if (blink(clock_, 1.2)) {
        painter_->textCentered("PRESS ENTER TO PLAY", kScreenWidth / 2, 430, 3, theme::kText);
    }
    if (best_ > 0) {
        painter_->textCentered("BEST " + std::to_string(best_), kScreenWidth / 2, 490, 2, theme::kGold);
    }
    painter_->textCentered("ESC TO QUIT", kScreenWidth / 2, 640, 2, theme::kTextDim);
}

void App::drawPause()
{
    drawOverlayCard(170);
    const int top = (kScreenHeight - 170) / 2;
    painter_->textCentered("PAUSED", kScreenWidth / 2, top + 40, 6, theme::kText);
    painter_->textCentered("P TO RESUME   R TO RESTART", kScreenWidth / 2, top + 112, 2, theme::kTextDim);
}

void App::drawGameOver()
{
    if (overTimer_ < 0.6) {
        return;
    }

    drawOverlayCard(280);
    const int top = (kScreenHeight - 280) / 2;
    const int cx = kScreenWidth / 2;

    painter_->textCentered("GAME OVER", cx, top + 34, 6, theme::kAccent);
    painter_->textCentered("SCORE", cx, top + 104, 2, theme::kTextDim);
    painter_->textCentered(std::to_string(game_.score()), cx, top + 128, 5, theme::kText);

    if (newBest_ && blink(clock_, 0.8)) {
        painter_->textCentered("NEW BEST!", cx, top + 182, 3, theme::kGold);
    }
    painter_->textCentered("ENTER TO PLAY AGAIN", cx, top + 232, 2, theme::kTextDim);
}
