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

enum class RightTab {
  kView2D,
  kTiles,
  kScripts,
  kCode,
};

enum class PaintTool {
  kPaint,
  kErase,
};

const char *TabLabel(RightTab tab) {
  switch (tab) {
    case RightTab::kView2D:
      return "2D View";
    case RightTab::kTiles:
      return "Tiles";
    case RightTab::kScripts:
      return "Scripts";
    case RightTab::kCode:
      return "Code";
  }
  return "";
}

std::string NextScriptName(const diskette16::Cart &cart) {
  for (int index = 1;; ++index) {
    const std::string name = "script_" + std::to_string(index);
    if (cart.FindScript(name) == nullptr) {
      return name;
    }
  }
}

void ResetCart(diskette16::Cart &cart,
               std::string &selected_script,
               std::string &script_buffer,
               int &active_tile,
               PaintTool &paint_tool,
               RightTab &active_tab,
               bool &script_focused) {
  cart = diskette16::Cart{};
  selected_script = "main";
  script_buffer = DefaultScript();
  cart.EnsureScript(selected_script, script_buffer);
  active_tile = 1;
  paint_tool = PaintTool::kPaint;
  active_tab = RightTab::kView2D;
  script_focused = false;
}

void DrawPanel(Rectangle rect, Color color) {
  DrawRectangleRec(rect, color);
  DrawRectangleLinesEx(rect, 1.0f, Color{60, 60, 72, 255});
}

void DrawTabButton(Rectangle rect, const char *label, bool active) {
  DrawRectangleRec(rect, active ? Color{54, 64, 88, 255} : Color{28, 32, 44, 255});
  DrawRectangleLinesEx(rect, 1.0f, active ? Color{255, 204, 112, 255} : Color{70, 74, 88, 255});
  const int text_width = MeasureText(label, 14);
  DrawText(label, static_cast<int>(rect.x + (rect.width - text_width) * 0.5f), static_cast<int>(rect.y + 8.0f), 14, Color{240, 236, 224, 255});
}

void DrawMiniPreview(const diskette16::Cart &cart, const TileSpriteAtlas &atlas, Rectangle rect) {
  DrawRectangleRec(rect, Color{12, 14, 20, 255});
  DrawRectangleLinesEx(rect, 1.0f, Color{70, 74, 88, 255});

  const float cell_width = rect.width / static_cast<float>(diskette16::Cart::kMapWidth);
  const float cell_height = rect.height / static_cast<float>(diskette16::Cart::kMapHeight);
  const float cell_size = std::min(cell_width, cell_height);

  for (int y = 0; y < diskette16::Cart::kMapHeight; ++y) {
    for (int x = 0; x < diskette16::Cart::kMapWidth; ++x) {
      const int tile = cart.tile(x, y);
      const Rectangle cell{rect.x + x * cell_size, rect.y + y * cell_size, cell_size, cell_size};
      DrawTileSprite(atlas, tile, cell);
    }
  }
}

}  // namespace

int main() {
  diskette16::Cart cart;
  diskette16::ScriptVM vm;
  std::string selected_script = "main";
  std::string script_buffer = DefaultScript();
  std::string last_error;
  int active_tile = 1;
  RightTab active_tab = RightTab::kView2D;
  PaintTool paint_tool = PaintTool::kPaint;
  bool script_focused = false;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(kWindowWidth, kWindowHeight, "Diskette16");
  SetTargetFPS(60);

  TileSpriteAtlas atlas = BuildTileAtlas(cart);
  cart.EnsureScript(selected_script, script_buffer);
  // Per-tile editable images kept in-memory in the editor.
  std::vector<Image> tile_images;
  tile_images.reserve(16);
  std::vector<bool> tile_custom;
  tile_custom.assign(16, false);
  for (int t = 0; t < 16; ++t) {
    Image img = GenImageColor(kSpriteSize, kSpriteSize, Color{0, 0, 0, 0});
    // Paint default glyph into the small tile image
    PaintGlyph(&img, t, cart, 0, 0);
    tile_images.push_back(img);
  }
  int edit_pixel_scale = 8;
  int paint_color_index = 1;

  while (!WindowShouldClose()) {
    const float screen_width = static_cast<float>(GetScreenWidth());
    const float screen_height = static_cast<float>(GetScreenHeight());
    const Rectangle left_panel{16.0f, 16.0f, 252.0f, screen_height - 32.0f};
    const Rectangle center_panel{280.0f, 16.0f, screen_width - 656.0f, screen_height - 32.0f};
    const Rectangle right_panel{screen_width - 360.0f, 16.0f, 344.0f, screen_height - 32.0f};
    const Rectangle canvas{center_panel.x + 24.0f, center_panel.y + 24.0f, static_cast<float>(kCanvasSize), static_cast<float>(kCanvasSize)};
    const Rectangle tab_bar{center_panel.x + 16.0f, center_panel.y + 8.0f, center_panel.width - 32.0f, 30.0f};
    const float tab_width = (tab_bar.width - 12.0f) / 4.0f;
    const Rectangle view_tab{tab_bar.x, tab_bar.y, tab_width, tab_bar.height};
    const Rectangle tiles_tab{tab_bar.x + tab_width + 4.0f, tab_bar.y, tab_width, tab_bar.height};
    const Rectangle scripts_tab{tab_bar.x + (tab_width + 4.0f) * 2.0f, tab_bar.y, tab_width, tab_bar.height};
    const Rectangle code_tab{tab_bar.x + (tab_width + 4.0f) * 3.0f, tab_bar.y, tab_width, tab_bar.height};
    const Vector2 mouse = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (PointInRect(mouse, view_tab)) {
        active_tab = RightTab::kView2D;
        script_focused = false;
      } else if (PointInRect(mouse, tiles_tab)) {
        active_tab = RightTab::kTiles;
        script_focused = false;
      } else if (PointInRect(mouse, scripts_tab)) {
        active_tab = RightTab::kScripts;
      } else if (PointInRect(mouse, code_tab)) {
        active_tab = RightTab::kCode;
      }
    }

    if (IsKeyPressed(KEY_F5)) {
      last_error.clear();
      cart.EnsureScript(selected_script, script_buffer);
      const bool ok = vm.RunNamed(selected_script, cart, &last_error);
      if (ok) {
        cart.pushLog("script executed successfully");
      } else {
        cart.pushLog("error: " + last_error);
      }
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)) {
      ResetCart(cart, selected_script, script_buffer, active_tile, paint_tool, active_tab, script_focused);
      last_error.clear();
    }

    // Map painting when in 2D view
    if (active_tab == RightTab::kView2D) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(mouse, canvas)) {
        const int x = static_cast<int>((mouse.x - canvas.x) / kCellSize);
        const int y = static_cast<int>((mouse.y - canvas.y) / kCellSize);
        if (x >= 0 && x < diskette16::Cart::kMapWidth && y >= 0 && y < diskette16::Cart::kMapHeight) {
          cart.tile(x, y) = paint_tool == PaintTool::kErase ? 0 : active_tile;
        }
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && PointInRect(mouse, canvas)) {
        const int x = static_cast<int>((mouse.x - canvas.x) / kCellSize);
        const int y = static_cast<int>((mouse.y - canvas.y) / kCellSize);
        if (x >= 0 && x < diskette16::Cart::kMapWidth && y >= 0 && y < diskette16::Cart::kMapHeight) {
          cart.tile(x, y) = 0;
        }
      }
    }

    // If in tile editor tab, handle pixel input & texture updates (no Draw calls here)
    Rectangle editor_area{};
    Rectangle draw_area{};
    int img_w = 0;
    int img_h = 0;
    if (active_tab == RightTab::kTiles) {
      editor_area = Rectangle{center_panel.x + 24.0f, center_panel.y + 24.0f, static_cast<float>(kCanvasSize), static_cast<float>(kCanvasSize)};
      const int tile = active_tile;
      Image &img = tile_images[static_cast<std::size_t>(tile)];
      img_w = img.width;
      img_h = img.height;
      const float draw_size = static_cast<float>(img_w * edit_pixel_scale);
      draw_area = Rectangle{editor_area.x + (editor_area.width - draw_size) * 0.5f, editor_area.y + (editor_area.height - draw_size) * 0.5f, draw_size, draw_size};

      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const Vector2 m = GetMousePosition();
        if (PointInRect(m, draw_area)) {
          const int local_x = static_cast<int>((m.x - draw_area.x) / edit_pixel_scale);
          const int local_y = static_cast<int>((m.y - draw_area.y) / edit_pixel_scale);
          if (local_x >= 0 && local_x < img_w && local_y >= 0 && local_y < img_h) {
            Color c = Color{cart.palette[static_cast<std::size_t>(paint_color_index)].r, cart.palette[static_cast<std::size_t>(paint_color_index)].g, cart.palette[static_cast<std::size_t>(paint_color_index)].b, 255};
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
              c = Color{0, 0, 0, 0};
            }
            ImageDrawRectangle(&img, local_x, local_y, 1, 1, c);
            tile_custom[static_cast<std::size_t>(tile)] = true;
            // Update the atlas region immediately
            UpdateTextureRec(atlas.texture, atlas.source_rects[static_cast<std::size_t>(tile)], img.data);
          }
        }
      }

      // Save/Reset click handling (visuals drawn later)
      const Rectangle save_btn{center_panel.x + 24.0f, center_panel.y + center_panel.height - 72.0f, 120.0f, 36.0f};
      const Rectangle reset_btn{center_panel.x + 156.0f, center_panel.y + center_panel.height - 72.0f, 120.0f, 36.0f};
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 m = GetMousePosition();
        if (PointInRect(m, reset_btn)) {
          Image new_img = GenImageColor(kSpriteSize, kSpriteSize, Color{0, 0, 0, 0});
          PaintGlyph(&new_img, active_tile, cart, 0, 0);
          UnloadImage(tile_images[static_cast<std::size_t>(active_tile)]);
          tile_images[static_cast<std::size_t>(active_tile)] = new_img;
          tile_custom[static_cast<std::size_t>(active_tile)] = false;
          UpdateTextureRec(atlas.texture, atlas.source_rects[static_cast<std::size_t>(active_tile)], new_img.data);
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

    // Center panel content depends on active tab

    DrawTabButton(view_tab, TabLabel(RightTab::kView2D), active_tab == RightTab::kView2D);
    DrawTabButton(tiles_tab, TabLabel(RightTab::kTiles), active_tab == RightTab::kTiles);
    DrawTabButton(scripts_tab, TabLabel(RightTab::kScripts), active_tab == RightTab::kScripts);
    DrawTabButton(code_tab, TabLabel(RightTab::kCode), active_tab == RightTab::kCode);

    // Center content: switch between map, pixel editor, and script editor
    if (active_tab == RightTab::kView2D) {
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
    } else if (active_tab == RightTab::kTiles) {
      // Pixel tile editor in center panel
      const Rectangle editor_area{center_panel.x + 24.0f, center_panel.y + 24.0f, static_cast<float>(kCanvasSize), static_cast<float>(kCanvasSize)};
      DrawRectangleRec(editor_area, Color{12, 14, 20, 255});

      const int tile = active_tile;
      const Image &img = tile_images[static_cast<std::size_t>(tile)];
      const float draw_size = static_cast<float>(img.width * edit_pixel_scale);
      const Rectangle draw_area{editor_area.x + (editor_area.width - draw_size) * 0.5f, editor_area.y + (editor_area.height - draw_size) * 0.5f, draw_size, draw_size};

      // Draw pixel grid
      for (int py = 0; py < img.height; ++py) {
        for (int px = 0; px < img.width; ++px) {
          unsigned char *pixels = static_cast<unsigned char *>(img.data);
          const std::size_t idx = static_cast<std::size_t>((py * img.width + px) * 4);
          Color pixel = Color{pixels[idx + 0], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]};
          const Rectangle cell{draw_area.x + px * edit_pixel_scale, draw_area.y + py * edit_pixel_scale, static_cast<float>(edit_pixel_scale), static_cast<float>(edit_pixel_scale)};
          DrawRectangleRec(cell, pixel);
          DrawRectangleLinesEx(cell, 1.0f, Color{10, 10, 12, 200});
        }
      }

      // Save/Reset buttons
      const Rectangle save_btn{center_panel.x + 24.0f, center_panel.y + center_panel.height - 72.0f, 120.0f, 36.0f};
      const Rectangle reset_btn{center_panel.x + 156.0f, center_panel.y + center_panel.height - 72.0f, 120.0f, 36.0f};
      DrawTabButton(save_btn, "Save", false);
      DrawTabButton(reset_btn, "Reset", false);
      DrawText(TextFormat("Editing tile %d", tile), static_cast<int>(center_panel.x + 24.0f), static_cast<int>(center_panel.y + 12.0f), 18, Color{240, 236, 224, 255});
    } else if (active_tab == RightTab::kScripts) {
      // Script editor in center panel
      const Rectangle script_area{center_panel.x + 24.0f, center_panel.y + 24.0f, center_panel.width - 48.0f, center_panel.height - 48.0f};
      DrawRectangleRec(script_area, script_focused ? Color{18, 24, 34, 255} : Color{15, 18, 25, 255});
      DrawRectangleLinesEx(script_area, 1.0f, script_focused ? Color{255, 204, 112, 255} : Color{70, 74, 88, 255});
      DrawTextEx(GetFontDefault(), script_buffer.c_str(), Vector2{script_area.x + 10.0f, script_area.y + 10.0f}, 16.0f, 1.0f, Color{232, 235, 240, 255});
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        script_focused = PointInRect(GetMousePosition(), script_area);
      }
    } else if (active_tab == RightTab::kCode) {
      // Code tab placeholder
      const Rectangle code_area{center_panel.x + 24.0f, center_panel.y + 24.0f, center_panel.width - 48.0f, center_panel.height - 48.0f};
      DrawRectangleRec(code_area, Color{16, 18, 22, 255});
      DrawRectangleLinesEx(code_area, 1.0f, Color{70, 74, 88, 255});
      DrawText("Code", static_cast<int>(code_area.x + 12.0f), static_cast<int>(code_area.y + 8.0f), 20, Color{240, 236, 224, 255});
      DrawText("Build/run settings and code hooks will appear here.", static_cast<int>(code_area.x + 12.0f), static_cast<int>(code_area.y + 40.0f), 16, Color{160, 166, 184, 255});
    }

    DrawText("F5 runs the selected script. Ctrl+N resets the cart.", static_cast<int>(center_panel.x + 16.0f), 56, 14, Color{160, 166, 184, 255});

    if (active_tab == RightTab::kView2D) {
      DrawText("2D View", static_cast<int>(left_panel.x + 16.0f), 420, 20, Color{240, 236, 224, 255});
      DrawMiniPreview(cart, atlas, Rectangle{left_panel.x + 16.0f, 520.0f, left_panel.width - 32.0f, 120.0f});
      DrawText(TextFormat("Tiles: %d  Scripts: %d  Entities: %d", diskette16::Cart::kPaletteSize, static_cast<int>(cart.scripts.size()), static_cast<int>(cart.entities.size())), static_cast<int>(left_panel.x + 16.0f), 660, 16, Color{204, 214, 221, 255});
      DrawText("This tab is the live overview.", static_cast<int>(left_panel.x + 16.0f), 684, 16, Color{160, 166, 184, 255});
    } else if (active_tab == RightTab::kTiles) {
      DrawText("Tile Editor", static_cast<int>(left_panel.x + 16.0f), 420, 20, Color{240, 236, 224, 255});
      DrawText("Paint or erase cells on the map.", static_cast<int>(left_panel.x + 16.0f), 448, 16, Color{160, 166, 184, 255});

      const Rectangle paint_button{left_panel.x + 16.0f, 476.0f, 140.0f, 30.0f};
      const Rectangle erase_button{left_panel.x + 164.0f, 476.0f, 140.0f, 30.0f};
      DrawTabButton(paint_button, "Paint", paint_tool == PaintTool::kPaint);
      DrawTabButton(erase_button, "Erase", paint_tool == PaintTool::kErase);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (PointInRect(mouse, paint_button)) {
          paint_tool = PaintTool::kPaint;
        } else if (PointInRect(mouse, erase_button)) {
          paint_tool = PaintTool::kErase;
        }
      }
      const Rectangle clear_button{left_panel.x + 16.0f, 516.0f, left_panel.width - 32.0f, 30.0f};
      DrawTabButton(clear_button, "Clear map", false);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(mouse, clear_button)) {
        cart.fill(0);
      }

      DrawText(TextFormat("Active tile: %d", active_tile), static_cast<int>(left_panel.x + 16.0f), 556, 18, Color{240, 236, 224, 255});
      DrawTileSprite(atlas, active_tile, Rectangle{left_panel.x + 16.0f, 586.0f, 52.0f, 52.0f});
      DrawText("Right click erases a cell.", static_cast<int>(left_panel.x + 16.0f), 650, 16, Color{160, 166, 184, 255});
    } else {
      DrawText("Scripts", static_cast<int>(left_panel.x + 16.0f), 420, 20, Color{240, 236, 224, 255});
      DrawText(TextFormat("Selected: %s", selected_script.c_str()), static_cast<int>(left_panel.x + 16.0f), 448, 16, Color{160, 166, 184, 255});

      const Rectangle source_box{left_panel.x + 16.0f, 476.0f, left_panel.width - 32.0f, 190.0f};
      DrawRectangleRec(source_box, script_focused ? Color{18, 24, 34, 255} : Color{15, 18, 25, 255});
      DrawRectangleLinesEx(source_box, 1.0f, script_focused ? Color{255, 204, 112, 255} : Color{70, 74, 88, 255});
      DrawTextEx(GetFontDefault(), script_buffer.c_str(), Vector2{source_box.x + 10.0f, source_box.y + 10.0f}, 16.0f, 1.0f, Color{232, 235, 240, 255});

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        script_focused = PointInRect(mouse, source_box);
      }

      if (script_focused) {
        int ch = GetCharPressed();
        while (ch > 0) {
          if (ch >= 32 && ch != 127) {
            script_buffer.push_back(static_cast<char>(ch));
          }
          ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_ENTER)) {
          script_buffer.push_back('\n');
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !script_buffer.empty()) {
          script_buffer.pop_back();
        }
      }

      cart.EnsureScript(selected_script, script_buffer);

      const Rectangle new_button{left_panel.x + 16.0f, 676.0f, left_panel.width - 32.0f, 30.0f};
      DrawTabButton(new_button, "New script", false);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(mouse, new_button)) {
        cart.EnsureScript(selected_script, script_buffer);
        selected_script = NextScriptName(cart);
        script_buffer.clear();
        cart.EnsureScript(selected_script, script_buffer);
        script_focused = true;
      }

      const Rectangle bind_tile_button{left_panel.x + 16.0f, 716.0f, left_panel.width - 32.0f, 30.0f};
      DrawTabButton(bind_tile_button, "Bind to active tile", false);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(mouse, bind_tile_button)) {
        cart.AddScriptToTile(active_tile, selected_script);
      }

      const Rectangle bind_entity_button{left_panel.x + 16.0f, 756.0f, left_panel.width - 32.0f, 30.0f};
      DrawTabButton(bind_entity_button, "Bind to player entity", false);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointInRect(mouse, bind_entity_button) && !cart.entities.empty()) {
        cart.AddScriptToEntity(0, selected_script);
      }

      DrawText("Bindings", static_cast<int>(left_panel.x + 16.0f), 796, 18, Color{240, 236, 224, 255});
      float binding_y = 824.0f;
      const auto &tile_bindings = cart.tile_scripts[static_cast<std::size_t>(active_tile)];
      DrawText(TextFormat("Tile %d:", active_tile), static_cast<int>(left_panel.x + 16.0f), static_cast<int>(binding_y), 16, Color{204, 214, 221, 255});
      binding_y += 20.0f;
      for (const auto &name : tile_bindings) {
        DrawTextEx(GetFontDefault(), name.c_str(), Vector2{left_panel.x + 24.0f, binding_y}, 16.0f, 1.0f, Color{160, 214, 184, 255});
        binding_y += 18.0f;
      }
    }

    DrawText("This starter keeps the editor tiny and cart-first.", static_cast<int>(left_panel.x + 16.0f), static_cast<int>(left_panel.y + left_panel.height - 24.0f), 14, Color{160, 166, 184, 255});

    EndDrawing();
  }

  UnloadTileAtlas(&atlas);
  CloseWindow();
  return 0;
}
