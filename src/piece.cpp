#include "piece.hpp"

namespace {

struct Shape {
    int boxSize;
    PieceCells cells;
};

// Spawn orientation of every piece, y grows downward.
constexpr std::array<Shape, kPieceTypeCount> kShapes{{
    {4, {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}}},  // I
    {2, {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}}},  // O
    {3, {{{1, 0}, {0, 1}, {1, 1}, {2, 1}}}},  // T
    {3, {{{1, 0}, {2, 0}, {0, 1}, {1, 1}}}},  // S
    {3, {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}}},  // Z
    {3, {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}}},  // J
    {3, {{{2, 0}, {0, 1}, {1, 1}, {2, 1}}}},  // L
}};

using RotationTable = std::array<std::array<PieceCells, 4>, kPieceTypeCount>;

RotationTable buildRotations()
{
    RotationTable table{};
    for (int type = 0; type < kPieceTypeCount; ++type) {
        const Shape& shape = kShapes[type];
        PieceCells cells = shape.cells;
        for (int rotation = 0; rotation < 4; ++rotation) {
            table[type][rotation] = cells;
            for (Offset& cell : cells) {
                cell = {shape.boxSize - 1 - cell.y, cell.x};
            }
        }
    }
    return table;
}

const RotationTable kRotations = buildRotations();

// Kick tables indexed by the starting rotation.
constexpr std::array<KickList, 4> kKicksClockwise{{
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},
    {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}},
    {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},
    {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}},
}};

constexpr std::array<KickList, 4> kKicksCounterClockwise{{
    {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},
    {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}},
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},
    {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}},
}};

constexpr std::array<KickList, 4> kLineKicksClockwise{{
    {{{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}}},
    {{{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}},
    {{{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}}},
    {{{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}}},
}};

constexpr std::array<KickList, 4> kLineKicksCounterClockwise{{
    {{{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}},
    {{{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}}},
    {{{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}}},
    {{{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}}},
}};

constexpr KickList kNoKick{{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}};

}  // namespace

const PieceCells& pieceCells(PieceType type, int rotation)
{
    return kRotations[static_cast<int>(type)][rotation & 3];
}

const KickList& wallKicks(PieceType type, int rotation, int direction)
{
    const int from = rotation & 3;
    switch (type) {
    case PieceType::O:
        return kNoKick;
    case PieceType::I:
        return direction > 0 ? kLineKicksClockwise[from] : kLineKicksCounterClockwise[from];
    default:
        return direction > 0 ? kKicksClockwise[from] : kKicksCounterClockwise[from];
    }
}
