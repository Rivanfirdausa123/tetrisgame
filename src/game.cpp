#include "game.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kLockDelay = 0.5;
constexpr int kMaxLockResets = 15;
constexpr double kSoftDropInterval = 0.03;
constexpr double kClearDuration = 0.32;
constexpr int kMaxLevel = 20;
constexpr int kLinesPerLevel = 10;
constexpr std::array<int, 5> kLineScores{0, 100, 300, 500, 800};

}  // namespace

Game::Game()
    : rng_(std::random_device{}())
{
    reset();
}

void Game::reset()
{
    board_.fill(0);
    queue_.clear();
    events_.clear();
    clearingRows_.clear();

    held_.reset();
    holdUsed_ = false;
    softDrop_ = false;

    state_ = State::Playing;
    fallTimer_ = 0.0;
    lockTimer_ = 0.0;
    clearTimer_ = 0.0;

    score_ = 0;
    lines_ = 0;
    level_ = 1;
    combo_ = -1;

    spawn(popQueue());
}

void Game::update(double dt)
{
    if (state_ == State::Clearing) {
        clearTimer_ += dt;
        if (clearTimer_ >= kClearDuration) {
            finishClear();
        }
        return;
    }

    if (state_ != State::Playing) {
        return;
    }

    if (grounded()) {
        fallTimer_ = 0.0;
        lockTimer_ += dt;
        if (lockTimer_ >= kLockDelay) {
            lockPiece();
        }
        return;
    }

    const double interval = softDrop_ ? std::min(gravityInterval(), kSoftDropInterval)
                                      : gravityInterval();
    fallTimer_ += dt;
    while (fallTimer_ >= interval) {
        fallTimer_ -= interval;
        if (!tryMove(0, 1)) {
            break;
        }
        if (softDrop_) {
            score_ += 1;
        }
        if (grounded()) {
            fallTimer_ = 0.0;
            break;
        }
    }
}

bool Game::shift(int dx)
{
    if (state_ != State::Playing || !tryMove(dx, 0)) {
        return false;
    }
    afterMove();
    return true;
}

bool Game::rotate(int direction)
{
    if (state_ != State::Playing || current_.type == PieceType::O) {
        return false;
    }

    const int target = (current_.rotation + direction + 4) % 4;
    for (const Offset& kick : wallKicks(current_.type, current_.rotation, direction)) {
        Piece candidate = current_;
        candidate.x += kick.x;
        candidate.y += kick.y;
        candidate.rotation = target;
        if (fits(candidate)) {
            current_ = candidate;
            afterMove();
            return true;
        }
    }
    return false;
}

void Game::hardDrop()
{
    if (state_ != State::Playing) {
        return;
    }
    const int distance = ghostY() - current_.y;
    current_.y += distance;
    score_ += distance * 2;
    events_.push_back({EventType::HardDrop, distance});
    lockPiece();
}

void Game::hold()
{
    if (state_ != State::Playing || holdUsed_) {
        return;
    }

    const PieceType outgoing = current_.type;
    if (held_) {
        spawn(*held_);
    } else {
        spawn(popQueue());
    }
    held_ = outgoing;
    holdUsed_ = true;
}

int Game::ghostY() const
{
    Piece probe = current_;
    while (true) {
        ++probe.y;
        if (!fits(probe)) {
            return probe.y - 1;
        }
    }
}

double Game::clearProgress() const
{
    return std::min(1.0, clearTimer_ / kClearDuration);
}

std::vector<Game::Event> Game::takeEvents()
{
    std::vector<Event> out;
    out.swap(events_);
    return out;
}

bool Game::fits(const Piece& piece) const
{
    for (const Offset& c : pieceCells(piece.type, piece.rotation)) {
        const int x = piece.x + c.x;
        const int y = piece.y + c.y;
        if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) {
            return false;
        }
        if (cell(x, y) != 0) {
            return false;
        }
    }
    return true;
}

bool Game::grounded() const
{
    Piece below = current_;
    ++below.y;
    return !fits(below);
}

bool Game::tryMove(int dx, int dy)
{
    Piece moved = current_;
    moved.x += dx;
    moved.y += dy;
    if (!fits(moved)) {
        return false;
    }
    current_ = moved;

    if (current_.y > lowestY_) {
        lowestY_ = current_.y;
        lockResets_ = 0;
        lockTimer_ = 0.0;
    }
    return true;
}

void Game::afterMove()
{
    if (grounded() && lockResets_ < kMaxLockResets) {
        lockTimer_ = 0.0;
        ++lockResets_;
    }
}

void Game::fillQueue()
{
    while (queue_.size() <= kPreviewCount) {
        std::array<PieceType, kPieceTypeCount> bag{};
        for (int i = 0; i < kPieceTypeCount; ++i) {
            bag[i] = static_cast<PieceType>(i);
        }
        std::shuffle(bag.begin(), bag.end(), rng_);
        queue_.insert(queue_.end(), bag.begin(), bag.end());
    }
}

PieceType Game::popQueue()
{
    fillQueue();
    const PieceType next = queue_.front();
    queue_.pop_front();
    fillQueue();
    return next;
}

void Game::spawn(PieceType type)
{
    current_ = {type, type == PieceType::O ? 4 : 3, 1, 0};
    if (!fits(current_)) {
        current_.y = 0;
    }

    fallTimer_ = 0.0;
    lockTimer_ = 0.0;
    lockResets_ = 0;
    lowestY_ = current_.y;

    if (!fits(current_)) {
        state_ = State::Over;
        events_.push_back({EventType::GameOver, score_});
    }
}

void Game::lockPiece()
{
    bool visible = false;
    for (const Offset& c : pieceCells(current_.type, current_.rotation)) {
        const int x = current_.x + c.x;
        const int y = current_.y + c.y;
        board_[y * kWidth + x] = static_cast<int>(current_.type) + 1;
        visible = visible || y >= kHiddenRows;
    }
    events_.push_back({EventType::Locked, 0});

    if (!visible) {
        state_ = State::Over;
        events_.push_back({EventType::GameOver, score_});
        return;
    }

    clearingRows_.clear();
    for (int y = 0; y < kHeight; ++y) {
        bool full = true;
        for (int x = 0; x < kWidth && full; ++x) {
            full = cell(x, y) != 0;
        }
        if (full) {
            clearingRows_.push_back(y);
        }
    }

    holdUsed_ = false;

    if (clearingRows_.empty()) {
        combo_ = -1;
        spawn(popQueue());
        return;
    }

    const int cleared = static_cast<int>(clearingRows_.size());
    ++combo_;
    score_ += kLineScores[cleared] * level_;
    if (combo_ > 0) {
        score_ += 50 * combo_ * level_;
    }

    lines_ += cleared;
    const int newLevel = std::min(kMaxLevel, 1 + lines_ / kLinesPerLevel);
    events_.push_back({EventType::LinesCleared, cleared});
    if (newLevel > level_) {
        level_ = newLevel;
        events_.push_back({EventType::LevelUp, level_});
    }

    state_ = State::Clearing;
    clearTimer_ = 0.0;
}

void Game::finishClear()
{
    int target = kHeight - 1;
    for (int y = kHeight - 1; y >= 0; --y) {
        if (std::find(clearingRows_.begin(), clearingRows_.end(), y) != clearingRows_.end()) {
            continue;
        }
        if (target != y) {
            std::copy_n(board_.begin() + y * kWidth, kWidth, board_.begin() + target * kWidth);
        }
        --target;
    }
    for (; target >= 0; --target) {
        std::fill_n(board_.begin() + target * kWidth, kWidth, 0);
    }

    clearingRows_.clear();
    state_ = State::Playing;
    spawn(popQueue());
}

double Game::gravityInterval() const
{
    const double base = 0.8 - (level_ - 1) * 0.007;
    return std::pow(base, level_ - 1);
}
