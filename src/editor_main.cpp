#include <raylib.h>

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <string>
#include <vector>

#include "cart.h"
#include "script_vm.h"

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr int kCellSize = 32;
constexpr int kCanvasSize = diskette16::Cart::kMapWidth * kCellSize;
constexpr int kSpritePadding = 4;
constexpr int kSpriteSize = 24;
constexpr int kAtlasColumns = 4;
constexpr int kAtlasRows = 4;
constexpr int kAtlasWidth = kAtlasColumns * (kSpriteSize + kSpritePadding) + kSpritePadding;
constexpr int kAtlasHeight = kAtlasRows * (kSpriteSize + kSpritePadding) + kSpritePadding;

struct TileSpriteAtlas {
  Texture2D texture{};
  std::vector<Rectangle> source_rects;
  bool ready = false;
};

struct TileGlyph {
  Color base;
  Color accent;
  Color outline;
  bool has_shadow = false;
};

Color Darken(Color color, unsigned char amount) {
  color.r = static_cast<unsigned char>(std::max(0, static_cast<int>(color.r) - amount));
  color.g = static_cast<unsigned char>(std::max(0, static_cast<int>(color.g) - amount));
  color.b = static_cast<unsigned char>(std::max(0, static_cast<int>(color.b) - amount));
  return color;
}

TileGlyph GlyphForTile(int tile, const diskette16::Cart &cart) {
  const diskette16::ColorRGBA source = cart.palette[static_cast<std::size_t>(tile % diskette16::Cart::kPaletteSize)];
  const Color base{source.r, source.g, source.b, 255};
  const Color accent = Color{static_cast<unsigned char>(std::min(255, source.r + 36)), static_cast<unsigned char>(std::min(255, source.g + 28)), static_cast<unsigned char>(std::min(255, source.b + 24)), 255};
  const Color outline = Darken(base, 70);

  switch (tile % 16) {
    case 0:
      return TileGlyph{Color{16, 18, 24, 255}, Color{26, 28, 36, 255}, Color{10, 12, 16, 255}, false};
    case 1:
      return TileGlyph{base, accent, outline, true};
    case 2:
      return TileGlyph{base, accent, outline, false};
    case 3:
      return TileGlyph{base, accent, outline, false};
    case 4:
      return TileGlyph{Color{72, 110, 66, 255}, Color{102, 148, 84, 255}, Color{42, 66, 38, 255}, false};
    case 5:
      return TileGlyph{Color{118, 86, 56, 255}, Color{172, 132, 72, 255}, Color{78, 52, 30, 255}, false};
    case 6:
      return TileGlyph{Color{54, 96, 156, 255}, Color{88, 142, 218, 255}, Color{24, 44, 78, 255}, false};
    case 7:
      return TileGlyph{Color{166, 72, 64, 255}, Color{224, 112, 96, 255}, Color{92, 34, 30, 255}, true};
    case 8:
      return TileGlyph{Color{184, 138, 48, 255}, Color{240, 186, 72, 255}, Color{96, 68, 20, 255}, false};
    case 9:
      return TileGlyph{Color{124, 106, 176, 255}, Color{164, 148, 226, 255}, Color{68, 58, 104, 255}, false};
    case 10:
      return TileGlyph{Color{68, 170, 110, 255}, Color{116, 228, 156, 255}, Color{26, 92, 54, 255}, false};
    case 11:
      return TileGlyph{Color{72, 144, 182, 255}, Color{124, 198, 236, 255}, Color{30, 72, 106, 255}, false};
    case 12:
      return TileGlyph{Color{82, 82, 100, 255}, Color{122, 122, 148, 255}, Color{42, 42, 52, 255}, false};
    case 13:
      return TileGlyph{Color{198, 92, 132, 255}, Color{248, 136, 176, 255}, Color{104, 42, 74, 255}, true};
    case 14:
      return TileGlyph{Color{220, 164, 104, 255}, Color{255, 214, 148, 255}, Color{120, 84, 48, 255}, false};
    default:
      return TileGlyph{Color{240, 240, 240, 255}, Color{255, 255, 255, 255}, Color{96, 96, 96, 255}, false};
  }
}

void PaintGlyph(Image *image, int tile, const diskette16::Cart &cart, int sprite_x, int sprite_y) {
  const TileGlyph glyph = GlyphForTile(tile, cart);
  const int left = sprite_x;
  const int top = sprite_y;
  const int right = sprite_x + kSpriteSize - 1;
  const int bottom = sprite_y + kSpriteSize - 1;

  ImageDrawRectangle(image, left, top, kSpriteSize, kSpriteSize, glyph.base);
  ImageDrawRectangleLines(image, Rectangle{static_cast<float>(left), static_cast<float>(top), static_cast<float>(kSpriteSize), static_cast<float>(kSpriteSize)}, 1, glyph.outline);

  if (tile == 0) {
    ImageDrawLine(image, left + 4, top + 4, right - 4, bottom - 4, glyph.accent);
    ImageDrawLine(image, right - 4, top + 4, left + 4, bottom - 4, glyph.accent);
    return;
  }

  if (glyph.has_shadow) {
    ImageDrawRectangle(image, left + 3, top + 4, kSpriteSize - 6, kSpriteSize - 7, Darken(glyph.base, 28));
  }

  switch (tile % 8) {
    case 1:
      ImageDrawRectangle(image, left + 5, top + 5, kSpriteSize - 10, kSpriteSize - 10, glyph.accent);
      ImageDrawRectangleLines(image, Rectangle{static_cast<float>(left + 5), static_cast<float>(top + 5), static_cast<float>(kSpriteSize - 10), static_cast<float>(kSpriteSize - 10)}, 1, glyph.outline);
      break;
    case 2:
      ImageDrawRectangle(image, left + 5, top + 8, kSpriteSize - 10, 5, glyph.accent);
      ImageDrawRectangle(image, left + 8, top + 5, 5, kSpriteSize - 10, glyph.accent);
      break;
    case 3:
      ImageDrawLine(image, left + 5, top + 7, right - 5, top + 7, glyph.accent);
      ImageDrawLine(image, left + 5, top + 12, right - 5, top + 12, glyph.accent);
      ImageDrawLine(image, left + 5, top + 17, right - 5, top + 17, glyph.accent);
      break;
    case 4:
      ImageDrawCircle(image, left + 12, top + 12, 7.0f, glyph.accent);
      ImageDrawLine(image, left + 4, top + 12, right - 4, top + 12, glyph.outline);
      break;
    case 5:
      ImageDrawTriangle(image, Vector2{static_cast<float>(left + 4), static_cast<float>(bottom - 4)}, Vector2{static_cast<float>(left + 12), static_cast<float>(top + 5)}, Vector2{static_cast<float>(right - 4), static_cast<float>(bottom - 4)}, glyph.accent);
      ImageDrawLine(image, left + 4, bottom - 4, left + 12, top + 5, glyph.outline);
      ImageDrawLine(image, left + 12, top + 5, right - 4, bottom - 4, glyph.outline);
      break;
    case 6:
      ImageDrawRectangle(image, left + 5, top + 5, 6, 6, glyph.accent);
      ImageDrawRectangle(image, right - 11, top + 5, 6, 6, glyph.accent);
      ImageDrawRectangle(image, left + 5, bottom - 11, 6, 6, glyph.accent);
      ImageDrawRectangle(image, right - 11, bottom - 11, 6, 6, glyph.accent);
      break;
    case 7:
      ImageDrawLine(image, left + 4, top + 4, right - 4, bottom - 4, glyph.accent);
      ImageDrawLine(image, left + 4, bottom - 4, right - 4, top + 4, glyph.accent);
      break;
    default:
      ImageDrawRectangle(image, left + 6, top + 6, kSpriteSize - 12, kSpriteSize - 12, glyph.accent);
      break;
  }
}

TileSpriteAtlas BuildTileAtlas(const diskette16::Cart &cart) {
  TileSpriteAtlas atlas;
  atlas.source_rects.reserve(16);

  Image image = GenImageColor(kAtlasWidth, kAtlasHeight, Color{0, 0, 0, 0});
  for (int tile = 0; tile < 16; ++tile) {
    const int column = tile % kAtlasColumns;
    const int row = tile / kAtlasColumns;
    const int sprite_x = kSpritePadding + column * (kSpriteSize + kSpritePadding);
    const int sprite_y = kSpritePadding + row * (kSpriteSize + kSpritePadding);
    PaintGlyph(&image, tile, cart, sprite_x, sprite_y);
    atlas.source_rects.push_back(Rectangle{static_cast<float>(sprite_x), static_cast<float>(sprite_y), static_cast<float>(kSpriteSize), static_cast<float>(kSpriteSize)});
  }

  atlas.texture = LoadTextureFromImage(image);
  UnloadImage(image);
  atlas.ready = true;
  return atlas;
}

void UnloadTileAtlas(TileSpriteAtlas *atlas) {
  if (atlas == nullptr || !atlas->ready) {
    return;
  }
  UnloadTexture(atlas->texture);
  atlas->ready = false;
}

void DrawTileSprite(const TileSpriteAtlas &atlas, int tile, Rectangle target) {
  if (!atlas.ready || tile < 0 || tile >= static_cast<int>(atlas.source_rects.size())) {
    DrawRectangleRec(target, Color{255, 0, 255, 255});
    return;
  }

  const Rectangle source = atlas.source_rects[static_cast<std::size_t>(tile)];
  DrawTexturePro(atlas.texture, source, target, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

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
  cart.EnsureScript("main", script);
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

  TileSpriteAtlas atlas = BuildTileAtlas(cart);

  while (!WindowShouldClose()) {
    const float screen_width = static_cast<float>(GetScreenWidth());
    const float screen_height = static_cast<float>(GetScreenHeight());
    const Rectangle left_panel{16.0f, 16.0f, 252.0f, screen_height - 32.0f};
    const Rectangle center_panel{280.0f, 16.0f, screen_width - 656.0f, screen_height - 32.0f};
    const Rectangle right_panel{screen_width - 360.0f, 16.0f, 344.0f, screen_height - 32.0f};
    const Rectangle canvas{center_panel.x + 24.0f, center_panel.y + 24.0f, static_cast<float>(kCanvasSize), static_cast<float>(kCanvasSize)};

    if (IsKeyPressed(KEY_F5)) {
      last_error.clear();
      cart.EnsureScript("main", script);
      const bool ok = vm.RunNamed("main", cart, &last_error);
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
      DrawRectangleRec(tile_button, Color{18, 22, 30, 255});
      DrawTileSprite(atlas, i, Rectangle{tile_button.x + 4.0f, tile_button.y + 4.0f, 36.0f, 36.0f});
      if (i == active_tile) {
        DrawRectangleLinesEx(tile_button, 3.0f, Color{255, 204, 112, 255});
      }
      DrawRectangleLinesEx(tile_button, 2.0f, Color{15, 15, 18, 255});

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), tile_button)) {
        active_tile = i;
      }
    }

    DrawText("Current tile", 32, 344, 18, Color{240, 236, 224, 255});
    DrawTileSprite(atlas, active_tile, Rectangle{32.0f, 370.0f, 48.0f, 48.0f});
    DrawText("Mouse wheel changes selection", 32, 406, 16, Color{160, 166, 184, 255});

    DrawText(cart.name.c_str(), static_cast<int>(canvas.x), static_cast<int>(center_panel.y + 4.0f), 20, Color{240, 236, 224, 255});
    DrawRectangleRec(canvas, Color{12, 14, 20, 255});

    for (int y = 0; y < diskette16::Cart::kMapHeight; ++y) {
      for (int x = 0; x < diskette16::Cart::kMapWidth; ++x) {
        const int tile = cart.tile(x, y);
        const Rectangle cell{canvas.x + x * kCellSize, canvas.y + y * kCellSize, static_cast<float>(kCellSize), static_cast<float>(kCellSize)};
        DrawTileSprite(atlas, tile, cell);
        DrawRectangleLinesEx(cell, 1.0f, Color{20, 20, 24, 255});
      }
    }

    DrawText("Left click to paint the tile map", static_cast<int>(canvas.x), static_cast<int>(canvas.y + kCanvasSize + 14.0f), 18, Color{160, 166, 184, 255});

    DrawText("Scripts", static_cast<int>(right_panel.x + 16.0f), 28, 20, Color{240, 236, 224, 255});
    DrawText("F5 runs `main`. Ctrl+N resets the cart.", static_cast<int>(right_panel.x + 16.0f), 56, 14, Color{160, 166, 184, 255});

    const Rectangle script_box{right_panel.x + 16.0f, 86.0f, right_panel.width - 32.0f, 280.0f};
    DrawRectangleRec(script_box, Color{15, 18, 25, 255});
    DrawRectangleLinesEx(script_box, 1.0f, Color{70, 74, 88, 255});
    DrawTextEx(GetFontDefault(), script.c_str(), Vector2{script_box.x + 10.0f, script_box.y + 10.0f}, 18.0f, 1.5f, Color{232, 235, 240, 255});

    float script_list_y = script_box.y + script_box.height + 16.0f;
    DrawText("Bound scripts", static_cast<int>(right_panel.x + 16.0f), static_cast<int>(script_list_y), 18, Color{240, 236, 224, 255});
    script_list_y += 26.0f;
    for (const auto &entry : cart.scripts) {
      DrawTextEx(GetFontDefault(), (entry.name + " -> " + std::to_string(entry.source.size()) + " bytes").c_str(), Vector2{right_panel.x + 16.0f, script_list_y}, 16.0f, 1.0f, Color{204, 214, 221, 255});
      script_list_y += 20.0f;
      if (script_list_y > right_panel.y + right_panel.height - 120.0f) {
        break;
      }
    }

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
  UnloadTileAtlas(&atlas);
  return 0;
}
