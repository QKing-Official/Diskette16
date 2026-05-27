#include <cassert>
#include <string>

#include "cart_format.h"

int RunCartFormatTest() {
  diskette16::Cart cart;
  cart.name = "Serialize Test";
  cart.script = "print hello\n";
  cart.tile(1, 1) = 5;

  const std::string text = diskette16::CartFormat::Serialize(cart);
  diskette16::Cart loaded;
  std::string error;
  const bool ok = diskette16::CartFormat::Deserialize(text, &loaded, &error);
  assert(ok);
  assert(error.empty());
  assert(loaded.name == "Serialize Test");
  assert(loaded.script.find("print hello") != std::string::npos);
  assert(loaded.tile(1, 1) == 5);
  return 0;
}
