#pragma once

#include "piece.hpp"

#include <SDL.h>

#include <array>

namespace theme {

constexpr SDL_Color kSkyTop{38, 30, 66, 255};
constexpr SDL_Color kSkyBottom{82, 52, 104, 255};

constexpr SDL_Color kBoard{24, 20, 40, 255};
constexpr SDL_Color kGrid{36, 31, 58, 255};
constexpr SDL_Color kPanel{30, 25, 50, 235};
constexpr SDL_Color kPanelBorder{88, 74, 132, 255};
constexpr SDL_Color kOverlay{14, 10, 26, 170};

constexpr SDL_Color kText{248, 244, 255, 255};
constexpr SDL_Color kTextDim{162, 150, 206, 255};
constexpr SDL_Color kAccent{255, 158, 181, 255};
constexpr SDL_Color kGold{255, 214, 120, 255};
constexpr SDL_Color kStone{92, 86, 118, 255};

constexpr std::array<SDL_Color, kPieceTypeCount> kPieces{{
    {112, 220, 230, 255},  // I  aqua
    {255, 216, 110, 255},  // O  lemon
    {190, 150, 250, 255},  // T  lavender
    {140, 228, 150, 255},  // S  mint
    {255, 130, 150, 255},  // Z  strawberry
    {120, 170, 255, 255},  // J  sky
    {255, 172, 110, 255},  // L  peach
}};

inline SDL_Color piece(PieceType type)
{
    return kPieces[static_cast<int>(type)];
}

}  // namespace theme
