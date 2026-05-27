#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "cart_format.h"

namespace {

bool WriteFile(const std::string &path, const std::string &text) {
  std::ofstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  file << text;
  return static_cast<bool>(file);
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: diskette16_packer <input.cart.txt> <output.d16c>\n";
    return 1;
  }

  const std::string input_path = argv[1];
  const std::string output_path = argv[2];

  std::ifstream input(input_path, std::ios::binary);
  if (!input.is_open()) {
    std::cerr << "failed to read input cart source\n";
    return 1;
  }

  diskette16::Cart cart;
  std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  std::istringstream lines(source);
  std::string line;
  enum class Mode { kNone, kScript, kMap } mode = Mode::kNone;
  std::string current_script_name = "main";
  int map_y = 0;

  while (std::getline(lines, line)) {
    const std::string trimmed = line;
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    if (trimmed.rfind("name ", 0) == 0) {
      cart.name = trimmed.substr(5);
      continue;
    }

    if (trimmed.rfind("script ", 0) == 0) {
      current_script_name = trimmed.substr(7);
      cart.EnsureScript(current_script_name, {});
      mode = Mode::kScript;
      continue;
    }
    if (trimmed == "end_script") {
      mode = Mode::kNone;
      continue;
    }
    if (trimmed.rfind("tile ", 0) == 0) {
      std::istringstream tile_line(trimmed.substr(5));
      int tile_id = 0;
      std::string script_name;
      if (tile_line >> tile_id >> script_name) {
        cart.AddScriptToTile(tile_id, script_name);
      }
      continue;
    }
    if (trimmed.rfind("entity ", 0) == 0) {
      std::istringstream entity_line(trimmed.substr(7));
      diskette16::Cart::Entity entity;
      if (entity_line >> entity.name >> entity.x >> entity.y >> entity.sprite_tile) {
        std::string script_name;
        while (entity_line >> script_name) {
          entity.scripts.push_back(script_name);
        }
        cart.entities.push_back(std::move(entity));
      }
      continue;
    }
    if (trimmed == "map") {
      mode = Mode::kMap;
      map_y = 0;
      continue;
    }
    if (trimmed == "end") {
      mode = Mode::kNone;
      continue;
    }

    if (mode == Mode::kScript) {
      auto &script = cart.EnsureScript(current_script_name, "");
      script.source.append(trimmed);
      script.source.push_back('\n');
      continue;
    }

    if (mode == Mode::kMap) {
      if (map_y >= diskette16::Cart::kMapHeight || static_cast<int>(trimmed.size()) != diskette16::Cart::kMapWidth) {
        std::cerr << "invalid map size\n";
        return 1;
      }
      for (int x = 0; x < diskette16::Cart::kMapWidth; ++x) {
        cart.tile(x, map_y) = trimmed[static_cast<std::size_t>(x)] - '0';
      }
      ++map_y;
      continue;
    }
  }

  const std::string packed = diskette16::CartFormat::Serialize(cart);
  if (!WriteFile(output_path, packed)) {
    std::cerr << "failed to write output cart\n";
    return 1;
  }

  std::cout << "packed " << input_path << " -> " << output_path << '\n';
  return 0;
}
