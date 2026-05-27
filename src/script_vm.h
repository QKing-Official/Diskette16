#pragma once

#include <string>

#include "cart.h"

namespace diskette16 {

class ScriptVM {
 public:
  bool Run(const std::string &source, Cart &cart, std::string *error = nullptr);
};

}  // namespace diskette16
