#pragma once

#include <string>
#include <vector>

#include "cart.h"

namespace diskette16 {

class ScriptVM {
 public:
  bool Run(const std::string &source, Cart &cart, std::string *error = nullptr);
  bool RunNamed(const std::string &script_name, Cart &cart, std::string *error = nullptr);
  bool RunMany(const std::vector<std::string> &script_names, Cart &cart, std::string *error = nullptr);
};

}  // namespace diskette16
