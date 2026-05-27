#include <cassert>
#include <string>

#include "cart_format.h"

int RunCartFormatTest() {
  diskette16::Cart cart;
  cart.name = "Serialize Test";
  cart.EnsureScript("main", "print hello\n");
  cart.AddScriptToTile(3, "main");
  cart.entities.push_back(diskette16::Cart::Entity{"hero", 2, 3, 1, {"main"}});
  cart.tile(1, 1) = 5;

  const std::string text = diskette16::CartFormat::Serialize(cart);
  diskette16::Cart loaded;
  std::string error;
  const bool ok = diskette16::CartFormat::Deserialize(text, &loaded, &error);
  assert(ok);
  assert(error.empty());
  assert(loaded.name == "Serialize Test");
  assert(loaded.FindScript("main") != nullptr);
  assert(loaded.FindScript("main")->source.find("print hello") != std::string::npos);
  assert(!loaded.tile_scripts[3].empty());
  assert(loaded.tile_scripts[3][0] == "main");
  bool found_hero = false;
  for (const auto &entity : loaded.entities) {
    if (entity.name == "hero") {
      found_hero = true;
      break;
    }
  }
  assert(found_hero);
  assert(loaded.tile(1, 1) == 5);
  return 0;
}
