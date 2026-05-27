#include "cart.h"

#include <algorithm>
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
  EnsureScript("init", "title Init\nprint init\n");
  EnsureScript("update", "print update\n");
  EnsureScript("player", "print player\n");
  entities.push_back(Entity{"player", 6, 6, 1, {"player"}});
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

Cart::ScriptSource *Cart::FindScript(const std::string &name) {
  for (auto &script : scripts) {
    if (script.name == name) {
      return &script;
    }
  }
  return nullptr;
}

const Cart::ScriptSource *Cart::FindScript(const std::string &name) const {
  for (const auto &script : scripts) {
    if (script.name == name) {
      return &script;
    }
  }
  return nullptr;
}

Cart::ScriptSource &Cart::EnsureScript(const std::string &name, std::string source) {
  if (ScriptSource *existing = FindScript(name); existing != nullptr) {
    if (!source.empty()) {
      existing->source = std::move(source);
    }
    return *existing;
  }

  scripts.push_back(ScriptSource{name, std::move(source)});
  return scripts.back();
}

void Cart::AddScriptToTile(int tile_id, const std::string &script_name) {
  if (tile_id < 0 || tile_id >= kPaletteSize) {
    return;
  }

  auto &bindings = tile_scripts[static_cast<std::size_t>(tile_id)];
  if (std::find(bindings.begin(), bindings.end(), script_name) == bindings.end()) {
    bindings.push_back(script_name);
  }
}

void Cart::AddScriptToEntity(std::size_t entity_index, const std::string &script_name) {
  if (entity_index >= entities.size()) {
    return;
  }

  auto &bindings = entities[entity_index].scripts;
  if (std::find(bindings.begin(), bindings.end(), script_name) == bindings.end()) {
    bindings.push_back(script_name);
  }
}

}  // namespace diskette16
