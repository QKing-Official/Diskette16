#include "script_vm.h"

#include <cctype>
#include <sstream>
#include <string_view>
#include <vector>

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

std::vector<std::string> SplitWords(const std::string &line) {
  std::istringstream stream(line);
  std::vector<std::string> words;
  std::string word;
  while (stream >> word) {
    words.push_back(word);
  }
  return words;
}

bool StartsWith(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

bool ParseInt(const std::string &text, int *value) {
  try {
    std::size_t parsed = 0;
    const int result = std::stoi(text, &parsed, 10);
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

bool ScriptVM::Run(const std::string &source, Cart &cart, std::string *error) {
  cart.clearLog();

  std::istringstream lines(source);
  std::string line;
  int line_number = 0;

  while (std::getline(lines, line)) {
    ++line_number;
    const std::string trimmed = Trim(line);
    if (trimmed.empty() || StartsWith(trimmed, "#") || StartsWith(trimmed, "//")) {
      continue;
    }

    const std::vector<std::string> words = SplitWords(trimmed);
    const std::string command = words.front();

    if (command == "print") {
      const std::size_t message_start = trimmed.find(' ');
      const std::string message = message_start == std::string::npos ? std::string{} : Trim(trimmed.substr(message_start + 1));
      cart.pushLog(message);
      continue;
    }

    if (command == "title") {
      const std::size_t title_start = trimmed.find(' ');
      cart.name = title_start == std::string::npos ? "Untitled Cart" : Trim(trimmed.substr(title_start + 1));
      continue;
    }

    if (command == "fill") {
      if (words.size() != 2) {
        if (error != nullptr) {
          *error = "line " + std::to_string(line_number) + ": fill expects 1 argument";
        }
        return false;
      }

      int value = 0;
      if (!ParseInt(words[1], &value)) {
        if (error != nullptr) {
          *error = "line " + std::to_string(line_number) + ": invalid tile id";
        }
        return false;
      }

      cart.fill(value);
      continue;
    }

    if (command == "set") {
      if (words.size() != 4) {
        if (error != nullptr) {
          *error = "line " + std::to_string(line_number) + ": set expects x y tile";
        }
        return false;
      }

      int x = 0;
      int y = 0;
      int tile = 0;
      if (!ParseInt(words[1], &x) || !ParseInt(words[2], &y) || !ParseInt(words[3], &tile)) {
        if (error != nullptr) {
          *error = "line " + std::to_string(line_number) + ": invalid set arguments";
        }
        return false;
      }

      if (x < 0 || x >= Cart::kMapWidth || y < 0 || y >= Cart::kMapHeight) {
        if (error != nullptr) {
          *error = "line " + std::to_string(line_number) + ": coordinates out of bounds";
        }
        return false;
      }

      cart.tile(x, y) = tile;
      continue;
    }

    if (command == "clear") {
      cart.fill(0);
      continue;
    }

    if (error != nullptr) {
      *error = "line " + std::to_string(line_number) + ": unknown command '" + command + "'";
    }
    return false;
  }

  return true;
}

}  // namespace diskette16
