#pragma once

#include <array>
#include <cstdint>

namespace font {

constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;
constexpr int kAdvance = kGlyphWidth + 1;

using Glyph = std::array<std::uint8_t, kGlyphHeight>;

// Returns nullptr for blank or unsupported characters.
const Glyph* glyph(char c);

}  // namespace font
