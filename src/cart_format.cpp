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
  out << "D16C2\n";
  out << "name " << cart.name << "\n";
  out << "scripts_begin\n";
  for (const auto &script : cart.scripts) {
    out << "script " << script.name << "\n";
    out << script.source;
    if (script.source.empty() || script.source.back() != '\n') {
      out << '\n';
    }
    out << "end_script\n";
  }
  out << "scripts_end\n";

  out << "tile_scripts_begin\n";
  for (int tile_id = 0; tile_id < Cart::kPaletteSize; ++tile_id) {
    const auto &bindings = cart.tile_scripts[static_cast<std::size_t>(tile_id)];
    out << "tile " << tile_id << ' ' << bindings.size();
    for (const std::string &name : bindings) {
      out << ' ' << name;
    }
    out << '\n';
  }
  out << "tile_scripts_end\n";

  out << "entities_begin\n";
  for (const auto &entity : cart.entities) {
    out << "entity " << entity.name << ' ' << entity.x << ' ' << entity.y << ' ' << entity.sprite_tile << ' ' << entity.scripts.size();
    for (const std::string &name : entity.scripts) {
      out << ' ' << name;
    }
    out << '\n';
  }
  out << "entities_end\n";

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

  if (!std::getline(input, line)) {
    if (error != nullptr) {
      *error = "missing cart header";
    }
    return false;
  }

  const std::string header = Trim(line);
  if (header != "D16C1" && header != "D16C2") {
    if (error != nullptr) {
      *error = "missing cart header";
    }
    return false;
  }

  bool in_scripts = false;
  bool in_map = false;
  bool in_tile_scripts = false;
  bool in_entities = false;
  int map_y = 0;
  cart->fill(0);
  cart->scripts.clear();
  cart->entities.clear();
  for (auto &bindings : cart->tile_scripts) {
    bindings.clear();
  }

  Cart::ScriptSource *current_script = nullptr;

  while (std::getline(input, line)) {
    const std::string trimmed = Trim(line);
    if (trimmed == "scripts_begin") {
      in_scripts = true;
      continue;
    }
    if (trimmed == "scripts_end") {
      in_scripts = false;
      current_script = nullptr;
      continue;
    }
    if (trimmed == "tile_scripts_begin") {
      in_tile_scripts = true;
      continue;
    }
    if (trimmed == "tile_scripts_end") {
      in_tile_scripts = false;
      continue;
    }
    if (trimmed == "entities_begin") {
      in_entities = true;
      continue;
    }
    if (trimmed == "entities_end") {
      in_entities = false;
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

    if (header == "D16C1") {
      if (trimmed == "script_begin") {
        in_scripts = true;
        current_script = &cart->EnsureScript("main", {});
        continue;
      }
      if (trimmed == "script_end") {
        in_scripts = false;
        current_script = nullptr;
        continue;
      }
    }

    if (in_scripts) {
      if (trimmed.rfind("script ", 0) == 0) {
        const std::string script_name = Trim(trimmed.substr(7));
        cart->scripts.push_back(Cart::ScriptSource{script_name, {}});
        current_script = &cart->scripts.back();
        continue;
      }

      if (trimmed == "end_script") {
        current_script = nullptr;
        continue;
      }

      if (current_script != nullptr) {
        current_script->source.append(line);
        current_script->source.push_back('\n');
      }
      continue;
    }

    if (in_tile_scripts) {
      if (trimmed.rfind("tile ", 0) != 0) {
        continue;
      }

      std::istringstream tile_line(trimmed.substr(5));
      int tile_id = 0;
      int binding_count = 0;
      if (!(tile_line >> tile_id >> binding_count) || tile_id < 0 || tile_id >= Cart::kPaletteSize) {
        if (error != nullptr) {
          *error = "invalid tile script binding";
        }
        return false;
      }

      auto &bindings = cart->tile_scripts[static_cast<std::size_t>(tile_id)];
      bindings.clear();
      for (int i = 0; i < binding_count; ++i) {
        std::string script_name;
        if (!(tile_line >> script_name)) {
          if (error != nullptr) {
            *error = "tile binding missing script names";
          }
          return false;
        }
        bindings.push_back(script_name);
      }
      continue;
    }

    if (in_entities) {
      if (trimmed.rfind("entity ", 0) != 0) {
        continue;
      }

      std::istringstream entity_line(trimmed.substr(7));
      Cart::Entity entity;
      std::size_t binding_count = 0;
      if (!(entity_line >> entity.name >> entity.x >> entity.y >> entity.sprite_tile >> binding_count)) {
        if (error != nullptr) {
          *error = "invalid entity line";
        }
        return false;
      }

      for (std::size_t i = 0; i < binding_count; ++i) {
        std::string script_name;
        if (!(entity_line >> script_name)) {
          if (error != nullptr) {
            *error = "entity binding missing script names";
          }
          return false;
        }
        entity.scripts.push_back(script_name);
      }

      cart->entities.push_back(std::move(entity));
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
