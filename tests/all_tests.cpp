#include <iostream>

int RunCartFormatTest();
int RunScriptVmTest();

int main() {
  RunScriptVmTest();
  RunCartFormatTest();
  std::cout << "all tests passed\n";
  return 0;
}
