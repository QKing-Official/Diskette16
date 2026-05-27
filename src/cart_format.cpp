#include "cart_format.h"

#include <cctype>
#include <sstream>
#include <string_view>

namespace diskette16 {
namespace {

std::string Trim(std::string_view view) {
  std::size_t begin = 0;
  while (begin < view.size() && std::isspace(static_cast<unsigned char>(view[begin])) != 0) {
    ++begin;
  }

  std::size_t end = view.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(view[end - 1])) != 0) {
    --end;
  }

  return std::string(view.substr(begin, end - begin));
}

bool ParseInt(std::string_view text, int *value) {
  try {
    std::size_t parsed = 0;
    const int result = std::stoi(std::string(text), &parsed, 10);
    if (parsed != text.size()) {
      return false;
    }
    *value = result;
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace

std::string CartFormat::Serialize(const Cart &cart) {
  std::ostringstream out;
  out << "D16C1\n";
  out << "name " << cart.name << "\n";
  out << "script_begin\n" << cart.script << "\nscript_end\n";
  out << "map_begin\n";
  for (int y = 0; y < Cart::kMapHeight; ++y) {
    for (int x = 0; x < Cart::kMapWidth; ++x) {
      out << cart.tile(x, y);
    }
    out << '\n';
  }
  out << "map_end\n";
  return out.str();
}

bool CartFormat::Deserialize(const std::string &text, Cart *cart, std::string *error) {
  if (cart == nullptr) {
    if (error != nullptr) {
      *error = "cart pointer was null";
    }
    return false;
  }

  std::istringstream input(text);
  std::string line;

  if (!std::getline(input, line) || Trim(line) != "D16C1") {
    if (error != nullptr) {
      *error = "missing D16C1 header";
    }
    return false;
  }

  bool in_script = false;
  bool in_map = false;
  int map_y = 0;
  cart->script.clear();
  cart->fill(0);

  while (std::getline(input, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed == "script_begin") {
      in_script = true;
      continue;
    }
    if (trimmed == "script_end") {
      in_script = false;
      continue;
    }
    if (trimmed == "map_begin") {
      in_map = true;
      map_y = 0;
      continue;
    }
    if (trimmed == "map_end") {
      in_map = false;
      continue;
    }

    if (trimmed.rfind("name ", 0) == 0) {
      cart->name = Trim(trimmed.substr(5));
      continue;
    }

    if (in_script) {
      cart->script.append(line);
      cart->script.push_back('\n');
      continue;
    }

    if (in_map) {
      if (map_y >= Cart::kMapHeight) {
        if (error != nullptr) {
          *error = "too many map rows";
        }
        return false;
      }

      if (trimmed.size() != static_cast<std::size_t>(Cart::kMapWidth)) {
        if (error != nullptr) {
          *error = "invalid map row width";
        }
        return false;
      }

      for (int x = 0; x < Cart::kMapWidth; ++x) {
        int value = 0;
        if (!ParseInt(trimmed.substr(static_cast<std::size_t>(x), 1), &value)) {
          if (error != nullptr) {
            *error = "map contained non-digit tiles";
          }
          return false;
        }
        cart->tile(x, map_y) = value;
      }

      ++map_y;
      continue;
    }
  }

  return true;
}

}  // namespace diskette16
