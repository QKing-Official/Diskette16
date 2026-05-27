#pragma once

#include <string>

#include "cart.h"

namespace diskette16 {

class CartFormat {
 public:
  static std::string Serialize(const Cart &cart);
  static bool Deserialize(const std::string &text, Cart *cart, std::string *error);
};

}  // namespace diskette16
