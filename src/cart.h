#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace diskette16 {

struct ColorRGBA {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
};

struct Cart {
  static constexpr int kMapWidth = 16;
  static constexpr int kMapHeight = 16;
  static constexpr int kPaletteSize = 16;

  std::string name = "Untitled Cart";
  std::string script;
  std::array<int, kMapWidth * kMapHeight> tiles{};
  std::array<ColorRGBA, kPaletteSize> palette{};
  std::vector<std::string> log;

  Cart();

  int &tile(int x, int y);
  const int &tile(int x, int y) const;
  void fill(int value);
  void clearLog();
  void pushLog(std::string message);
};

}  // namespace diskette16
