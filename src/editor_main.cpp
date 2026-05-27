#include <raylib.h>

#include <algorithm>
#include <cstddef>
#include <string>

#include "cart.h"
#include "script_vm.h"

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr int kCellSize = 32;
constexpr int kCanvasSize = diskette16::Cart::kMapWidth * kCellSize;

bool PointInRect(Vector2 point, Rectangle rect) {
  return CheckCollisionPointRec(point, rect);
}

std::string DefaultScript() {
  return R"(# Diskette16 cart script
title Starter Cart
fill 0
set 3 3 2
set 4 3 2
set 5 3 2
set 3 4 2
set 3 5 2
print Hello from Diskette16
)";
}

void ResetCart(diskette16::Cart &cart, std::string &script, int &active_tile) {
  cart = diskette16::Cart{};
  script = DefaultScript();
  active_tile = 1;
}

void DrawPanel(Rectangle rect, Color color) {
  DrawRectangleRec(rect, color);
  DrawRectangleLinesEx(rect, 1.0f, Color{60, 60, 72, 255});
}

}  // namespace

int main() {
  diskette16::Cart cart;
  diskette16::ScriptVM vm;
  std::string script = DefaultScript();
  std::string last_error;
  int active_tile = 1;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(kWindowWidth, kWindowHeight, "Diskette16");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_F5)) {
      last_error.clear();
      cart.script = script;
      const bool ok = vm.Run(script, cart, &last_error);
      if (ok) {
        cart.pushLog("script executed successfully");
      } else {
        cart.pushLog("error: " + last_error);
      }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)) {
      ResetCart(cart, script, active_tile);
      last_error.clear();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      const Rectangle canvas{24.0f, 24.0f, static_cast<float>(kCanvasSize), static_cast<float>(kCanvasSize)};
      if (PointInRect(mouse, canvas)) {
        const int x = static_cast<int>((mouse.x - canvas.x) / kCellSize);
        const int y = static_cast<int>((mouse.y - canvas.y) / kCellSize);
        if (x >= 0 && x < diskette16::Cart::kMapWidth && y >= 0 && y < diskette16::Cart::kMapHeight) {
          cart.tile(x, y) = active_tile;
        }
      }
    }

    const int wheel = static_cast<int>(GetMouseWheelMove());
    if (wheel != 0) {
      active_tile = std::clamp(active_tile + wheel, 0, 15);
    }

    BeginDrawing();
    ClearBackground(Color{18, 20, 28, 255});

    const float screen_width = static_cast<float>(GetScreenWidth());
    const float screen_height = static_cast<float>(GetScreenHeight());
    const Rectangle left_panel{16.0f, 16.0f, 252.0f, screen_height - 32.0f};
    const Rectangle center_panel{280.0f, 16.0f, screen_width - 656.0f, screen_height - 32.0f};
    const Rectangle right_panel{screen_width - 360.0f, 16.0f, 344.0f, screen_height - 32.0f};

    DrawPanel(left_panel, Color{25, 29, 41, 255});
    DrawPanel(center_panel, Color{22, 26, 36, 255});
    DrawPanel(right_panel, Color{25, 29, 41, 255});

    DrawText("Diskette16", 32, 28, 24, Color{240, 236, 224, 255});
    DrawText("native retro editor", 32, 56, 16, Color{160, 166, 184, 255});

    DrawText("Tile bank", 32, 92, 20, Color{240, 236, 224, 255});
    for (int i = 0; i < 16; ++i) {
      const int col = i % 4;
      const int row = i / 4;
      const Rectangle tile_button{32.0f + col * 52.0f, 128.0f + row * 52.0f, 44.0f, 44.0f};
      const Color fill = (i == active_tile) ? Color{255, 204, 112, 255} : Color{static_cast<unsigned char>(20 + i * 10), static_cast<unsigned char>(60 + i * 5), static_cast<unsigned char>(100 + i * 2), 255};
      DrawRectangleRec(tile_button, fill);
      DrawRectangleLinesEx(tile_button, 2.0f, Color{15, 15, 18, 255});
      DrawText(TextFormat("%d", i), static_cast<int>(tile_button.x + 15), static_cast<int>(tile_button.y + 12), 20, Color{15, 15, 18, 255});

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), tile_button)) {
        active_tile = i;
      }
    }

    DrawText("Current tile", 32, 344, 18, Color{240, 236, 224, 255});
    DrawText(TextFormat("%d", active_tile), 32, 370, 22, Color{255, 204, 112, 255});
    DrawText("Mouse wheel changes selection", 32, 406, 16, Color{160, 166, 184, 255});

    const Rectangle canvas{center_panel.x + 24.0f, center_panel.y + 24.0f, static_cast<float>(kCanvasSize), static_cast<float>(kCanvasSize)};
    DrawText(cart.name.c_str(), static_cast<int>(canvas.x), static_cast<int>(center_panel.y + 4.0f), 20, Color{240, 236, 224, 255});
    DrawRectangleRec(canvas, Color{12, 14, 20, 255});

    for (int y = 0; y < diskette16::Cart::kMapHeight; ++y) {
      for (int x = 0; x < diskette16::Cart::kMapWidth; ++x) {
        const int tile = cart.tile(x, y);
        const Rectangle cell{canvas.x + x * kCellSize, canvas.y + y * kCellSize, static_cast<float>(kCellSize), static_cast<float>(kCellSize)};
        const diskette16::ColorRGBA palette = cart.palette[static_cast<std::size_t>(tile % diskette16::Cart::kPaletteSize)];
        DrawRectangleRec(cell, Color{palette.r, palette.g, palette.b, 255});
        DrawRectangleLinesEx(cell, 1.0f, Color{20, 20, 24, 255});
        if (tile != 0) {
          DrawText(TextFormat("%d", tile), static_cast<int>(cell.x + 10), static_cast<int>(cell.y + 8), 16, Color{16, 16, 20, 255});
        }
      }
    }

    DrawText("Left click to paint the tile map", static_cast<int>(canvas.x), static_cast<int>(canvas.y + kCanvasSize + 14.0f), 18, Color{160, 166, 184, 255});

    DrawText("Cart script", static_cast<int>(right_panel.x + 16.0f), 28, 20, Color{240, 236, 224, 255});
    DrawText("F5 runs the custom script. Ctrl+N resets the cart.", static_cast<int>(right_panel.x + 16.0f), 56, 14, Color{160, 166, 184, 255});

    const Rectangle script_box{right_panel.x + 16.0f, 86.0f, right_panel.width - 32.0f, 280.0f};
    DrawRectangleRec(script_box, Color{15, 18, 25, 255});
    DrawRectangleLinesEx(script_box, 1.0f, Color{70, 74, 88, 255});
    DrawTextEx(GetFontDefault(), script.c_str(), Vector2{script_box.x + 10.0f, script_box.y + 10.0f}, 18.0f, 1.5f, Color{232, 235, 240, 255});

    DrawText("Log", static_cast<int>(right_panel.x + 16.0f), 384, 20, Color{240, 236, 224, 255});
    const Rectangle log_box{right_panel.x + 16.0f, 414.0f, right_panel.width - 32.0f, right_panel.height - 430.0f};
    DrawRectangleRec(log_box, Color{15, 18, 25, 255});
    DrawRectangleLinesEx(log_box, 1.0f, Color{70, 74, 88, 255});

    float log_y = log_box.y + 10.0f;
    for (const std::string &entry : cart.log) {
      DrawTextEx(GetFontDefault(), entry.c_str(), Vector2{log_box.x + 10.0f, log_y}, 16.0f, 1.0f, Color{204, 214, 221, 255});
      log_y += 20.0f;
      if (log_y > log_box.y + log_box.height - 20.0f) {
        break;
      }
    }

    DrawText("This starter keeps the editor tiny and cart-first.", static_cast<int>(right_panel.x + 16.0f), static_cast<int>(right_panel.y + right_panel.height - 24.0f), 14, Color{160, 166, 184, 255});

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
