#pragma once

#include "piece.hpp"

#include <array>
#include <deque>
#include <optional>
#include <random>
#include <vector>

class Game {
public:
    static constexpr int kWidth = 10;
    static constexpr int kHeight = 22;
    static constexpr int kHiddenRows = 2;
    static constexpr int kPreviewCount = 3;

    enum class State { Playing, Clearing, Over };

    enum class EventType { Locked, HardDrop, LinesCleared, LevelUp, GameOver };

    struct Event {
        EventType type;
        int value;
    };

    struct Piece {
        PieceType type;
        int x;
        int y;
        int rotation;
    };

    Game();

    void reset();
    void update(double dt);

    bool shift(int dx);
    bool rotate(int direction);
    void hardDrop();
    void hold();
    void setSoftDrop(bool enabled) { softDrop_ = enabled; }

    State state() const { return state_; }
    int cell(int x, int y) const { return board_[y * kWidth + x]; }
    const Piece& current() const { return current_; }
    int ghostY() const;

    std::optional<PieceType> heldPiece() const { return held_; }
    bool canHold() const { return !holdUsed_; }
    PieceType preview(int index) const { return queue_[index]; }

    int score() const { return score_; }
    int lines() const { return lines_; }
    int level() const { return level_; }
    int combo() const { return combo_; }

    const std::vector<int>& clearingRows() const { return clearingRows_; }
    double clearProgress() const;

    std::vector<Event> takeEvents();

private:
    bool fits(const Piece& piece) const;
    bool grounded() const;
    bool tryMove(int dx, int dy);
    void afterMove();

    void fillQueue();
    PieceType popQueue();
    void spawn(PieceType type);
    void lockPiece();
    void finishClear();
    double gravityInterval() const;

    std::array<int, kWidth * kHeight> board_{};
    std::deque<PieceType> queue_;
    std::mt19937 rng_;

    Piece current_{};
    std::optional<PieceType> held_;
    bool holdUsed_ = false;
    bool softDrop_ = false;

    State state_ = State::Playing;
    double fallTimer_ = 0.0;
    double lockTimer_ = 0.0;
    double clearTimer_ = 0.0;
    int lockResets_ = 0;
    int lowestY_ = 0;

    int score_ = 0;
    int lines_ = 0;
    int level_ = 1;
    int combo_ = -1;

    std::vector<int> clearingRows_;
    std::vector<Event> events_;
};
