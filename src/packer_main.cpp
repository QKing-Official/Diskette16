#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "cart_format.h"

namespace {

std::string ReadFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return {};
  }

  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

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

  const std::string source = ReadFile(input_path);
  if (source.empty()) {
    std::cerr << "failed to read input cart source\n";
    return 1;
  }

  diskette16::Cart cart;
  std::istringstream lines(source);
  std::string line;
  enum class Mode { kNone, kScript, kMap } mode = Mode::kNone;
  int map_y = 0;

  while (std::getline(lines, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    if (line.rfind("name ", 0) == 0) {
      cart.name = line.substr(5);
      continue;
    }
    if (line == "script") {
      mode = Mode::kScript;
      continue;
    }
    if (line == "map") {
      mode = Mode::kMap;
      map_y = 0;
      continue;
    }
    if (line == "end") {
      mode = Mode::kNone;
      continue;
    }

    if (mode == Mode::kScript) {
      cart.script.append(line);
      cart.script.push_back('\n');
      continue;
    }

    if (mode == Mode::kMap) {
      if (map_y >= diskette16::Cart::kMapHeight || static_cast<int>(line.size()) != diskette16::Cart::kMapWidth) {
        std::cerr << "invalid map size\n";
        return 1;
      }
      for (int x = 0; x < diskette16::Cart::kMapWidth; ++x) {
        cart.tile(x, map_y) = line[static_cast<std::size_t>(x)] - '0';
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
