#pragma once

#include <array>
#include <cstdint>

enum class PieceType : std::uint8_t { I, O, T, S, Z, J, L };

constexpr int kPieceTypeCount = 7;

struct Offset {
    int x;
    int y;
};

using PieceCells = std::array<Offset, 4>;
using KickList = std::array<Offset, 5>;

// Cells of a piece inside its bounding box for the given rotation (0..3).
const PieceCells& pieceCells(PieceType type, int rotation);

// Wall kick candidates for rotating from `rotation` in `direction` (+1 cw, -1 ccw).
const KickList& wallKicks(PieceType type, int rotation, int direction);
