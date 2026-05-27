#include <cassert>
#include <string>

#include "cart.h"
#include "script_vm.h"

int RunScriptVmTest() {
  diskette16::Cart cart;
  diskette16::ScriptVM vm;
  std::string error;

  const std::string script = R"(
title Test Cart
fill 1
set 2 3 7
print hello
)";

  const bool ok = vm.Run(script, cart, &error);
  assert(ok);
  assert(error.empty());
  assert(cart.name == "Test Cart");
  assert(cart.tile(2, 3) == 7);
  assert(cart.log.size() == 1);
  assert(cart.log.front() == "hello");
  return 0;
}
