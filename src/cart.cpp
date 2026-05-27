#include "cart.h"

#include <utility>

namespace diskette16 {

Cart::Cart() {
  palette = {
      ColorRGBA{29, 43, 83, 255},   ColorRGBA{126, 37, 83, 255},
      ColorRGBA{0, 135, 81, 255},    ColorRGBA{171, 82, 54, 255},
      ColorRGBA{95, 87, 79, 255},    ColorRGBA{194, 195, 199, 255},
      ColorRGBA{255, 241, 232, 255}, ColorRGBA{255, 0, 77, 255},
      ColorRGBA{255, 163, 0, 255},   ColorRGBA{255, 236, 39, 255},
      ColorRGBA{0, 228, 54, 255},    ColorRGBA{41, 173, 255, 255},
      ColorRGBA{131, 118, 156, 255}, ColorRGBA{255, 119, 168, 255},
      ColorRGBA{255, 204, 170, 255}, ColorRGBA{255, 255, 255, 255},
  };
  fill(0);
}

int &Cart::tile(int x, int y) {
  return tiles.at(static_cast<std::size_t>(y * kMapWidth + x));
}

const int &Cart::tile(int x, int y) const {
  return tiles.at(static_cast<std::size_t>(y * kMapWidth + x));
}

void Cart::fill(int value) {
  tiles.fill(value);
}

void Cart::clearLog() {
  log.clear();
}

void Cart::pushLog(std::string message) {
  log.push_back(std::move(message));
}

}  // namespace diskette16
