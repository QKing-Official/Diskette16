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

  struct ScriptSource {
    std::string name;
    std::string source;
  };

  struct Entity {
    std::string name = "entity";
    int x = 0;
    int y = 0;
    int sprite_tile = 1;
    std::vector<std::string> scripts;
  };

  std::string name = "Untitled Cart";
  std::vector<ScriptSource> scripts;
  std::array<int, kMapWidth * kMapHeight> tiles{};
  std::array<ColorRGBA, kPaletteSize> palette{};
  std::array<std::vector<std::string>, kPaletteSize> tile_scripts;
  std::vector<Entity> entities;
  std::vector<std::string> log;

  Cart();

  int &tile(int x, int y);
  const int &tile(int x, int y) const;
  void fill(int value);
  void clearLog();
  void pushLog(std::string message);

  ScriptSource *FindScript(const std::string &name);
  const ScriptSource *FindScript(const std::string &name) const;
  ScriptSource &EnsureScript(const std::string &name, std::string source = {});
  void AddScriptToTile(int tile_id, const std::string &script_name);
  void AddScriptToEntity(std::size_t entity_index, const std::string &script_name);
};

}  // namespace diskette16
