# Diskette16

Diskette16 is a lightweight retro 2D engine and editor for native Windows and Linux builds, with a cart format that behaves like a tiny diskette image.

## What this starter includes

- C++17 core
- CMake build
- Native editor prototype using raylib
- Custom line-based script VM
- Diskette cart format (`.d16c`)
- Separate packer that turns source assets into a cart file

## Build on WSL/Linux

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
./build/diskette16_editor
```

## Pack a cart

```bash
./build/diskette16_packer examples/starter.cart.txt build/starter.d16c
```

## Repo

Remote origin:

`https://github.com/QKing-Official/Diskette16`
