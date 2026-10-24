#include "sdl_gfx.h"

#include <SDL.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <string.h>

#include "font5x7.h"

#define GLYPH_CELL_W 6
#define GLYPH_CELL_H 8
#define GLYPH_FIRST 32
#define GLYPH_COUNT 96
#define GLYPH_COLUMNS 16

struct gfx {
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* font;
  float scale;
  float view_width;
};

static SDL_Texture* build_font(SDL_Renderer* renderer) {
  int width = GLYPH_COLUMNS * GLYPH_CELL_W;
  int rows = (GLYPH_COUNT + GLYPH_COLUMNS - 1) / GLYPH_COLUMNS;
  int height = rows * GLYPH_CELL_H;
  uint32_t* pixels = calloc((size_t)width * (size_t)height, sizeof(uint32_t));
  if (!pixels) return NULL;
  for (int i = 0; i < GLYPH_COUNT; i++) {
    char c = (char)(GLYPH_FIRST + i);
    int ox = (i % GLYPH_COLUMNS) * GLYPH_CELL_W;
    int oy = (i / GLYPH_COLUMNS) * GLYPH_CELL_H;
    for (int row = 0; row < FONT5X7_HEIGHT; row++)
      for (int col = 0; col < FONT5X7_WIDTH; col++)
        if (font5x7_pixel(c, row, col))
          pixels[(oy + row) * width + ox + col] = 0xffffffffu;
  }
  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
      pixels, width, height, 32, width * 4, 0x000000ffu, 0x0000ff00u,
      0x00ff0000u, 0xff000000u);
  SDL_Texture* texture = NULL;
  if (surface) {
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
  }
  free(pixels);
  if (texture) SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
  return texture;
}

gfx_t* gfx_open(const char* title, int width, int height) {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return NULL;
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  gfx_t* gfx = calloc(1, sizeof(*gfx));
  if (!gfx) return NULL;
  SDL_Rect usable = {0, 0, width, height};
  if (width <= 0 || height <= 0) SDL_GetDisplayUsableBounds(0, &usable);
  gfx->window = SDL_CreateWindow(
      title, usable.x, usable.y, usable.w, usable.h,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
  if (!gfx->window) {
    free(gfx);
    return NULL;
  }
  if (width <= 0 || height <= 0) {
    int top = 0, left = 0, bottom = 0, right = 0;
    if (SDL_GetWindowBordersSize(gfx->window, &top, &left, &bottom, &right) ==
        0) {
      SDL_SetWindowSize(gfx->window, usable.w - left - right,
                        usable.h - top - bottom);
      SDL_SetWindowPosition(gfx->window, usable.x + left, usable.y + top);
    }
  }
  gfx->renderer = SDL_CreateRenderer(
      gfx->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!gfx->renderer) gfx->renderer = SDL_CreateRenderer(gfx->window, -1, 0);
  if (!gfx->renderer) {
    SDL_DestroyWindow(gfx->window);
    free(gfx);
    return NULL;
  }
  SDL_SetRenderDrawBlendMode(gfx->renderer, SDL_BLENDMODE_BLEND);
  gfx->font = build_font(gfx->renderer);
  return gfx;
}

void gfx_close(gfx_t* gfx) {
  if (!gfx) return;
  if (gfx->font) SDL_DestroyTexture(gfx->font);
  SDL_DestroyRenderer(gfx->renderer);
  SDL_DestroyWindow(gfx->window);
  free(gfx);
}

void gfx_begin(gfx_t* gfx, uint8_t r, uint8_t g, uint8_t b) {
  int out_w = 1, out_h = 1;
  SDL_GetRendererOutputSize(gfx->renderer, &out_w, &out_h);
  gfx->scale = (float)out_h / GFX_VIEW_HEIGHT;
  gfx->view_width = (float)out_w / gfx->scale;
  SDL_RenderSetScale(gfx->renderer, gfx->scale, gfx->scale);
  SDL_SetRenderDrawColor(gfx->renderer, r, g, b, 255);
  SDL_RenderClear(gfx->renderer);
}

void gfx_end(gfx_t* gfx) { SDL_RenderPresent(gfx->renderer); }

float gfx_view_width(const gfx_t* gfx) { return gfx->view_width; }

float gfx_scale(const gfx_t* gfx) { return gfx->scale; }

void gfx_toggle_fullscreen(gfx_t* gfx) {
  Uint32 flags = SDL_GetWindowFlags(gfx->window);
  SDL_SetWindowFullscreen(
      gfx->window, (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                       ? 0
                       : SDL_WINDOW_FULLSCREEN_DESKTOP);
}

bool gfx_save_bmp(gfx_t* gfx, const char* path) {
  int w = 0, h = 0;
  SDL_GetRendererOutputSize(gfx->renderer, &w, &h);
  SDL_Surface* surface =
      SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_ARGB8888);
  if (!surface) return false;
  bool ok = SDL_RenderReadPixels(gfx->renderer, NULL, SDL_PIXELFORMAT_ARGB8888,
                                 surface->pixels, surface->pitch) == 0 &&
            SDL_SaveBMP(surface, path) == 0;
  SDL_FreeSurface(surface);
  return ok;
}

static texture_t wrap(SDL_Texture* handle, bool smooth) {
  texture_t texture = {NULL, 0, 0};
  if (!handle) return texture;
  SDL_QueryTexture(handle, NULL, NULL, &texture.w, &texture.h);
  SDL_SetTextureScaleMode(handle,
                          smooth ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
  texture.handle = handle;
  return texture;
}

texture_t gfx_texture_from_pixels(gfx_t* gfx, const uint32_t* pixels, int w,
                                  int h, bool smooth, bool additive) {
  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
      (void*)pixels, w, h, 32, w * 4, 0x000000ffu, 0x0000ff00u, 0x00ff0000u,
      0xff000000u);
  SDL_Texture* handle = NULL;
  if (surface) {
    handle = SDL_CreateTextureFromSurface(gfx->renderer, surface);
    SDL_FreeSurface(surface);
  }
  if (handle && additive) SDL_SetTextureBlendMode(handle, SDL_BLENDMODE_ADD);
  return wrap(handle, smooth);
}

texture_t gfx_texture_from_file(gfx_t* gfx, const char* path, bool smooth) {
  SDL_Surface* surface = IMG_Load(path);
  SDL_Texture* handle = NULL;
  if (surface) {
    handle = SDL_CreateTextureFromSurface(gfx->renderer, surface);
    SDL_FreeSurface(surface);
  }
  return wrap(handle, smooth);
}

void gfx_texture_free(texture_t* texture) {
  if (texture->handle) SDL_DestroyTexture(texture->handle);
  texture->handle = NULL;
}

void gfx_draw(gfx_t* gfx, texture_t texture, const rect_t* src, rect_t dst,
              const draw_t* options) {
  if (!texture.handle) return;
  SDL_Rect source;
  if (src) {
    source.x = (int)src->x;
    source.y = (int)src->y;
    source.w = (int)src->w;
    source.h = (int)src->h;
  }
  SDL_FRect target = {dst.x, dst.y, dst.w, dst.h};
  double angle = 0;
  SDL_RendererFlip flip = SDL_FLIP_NONE;
  Uint8 alpha = 255;
  Uint8 r = 255, g = 255, b = 255;
  if (options) {
    angle = options->angle;
    if (options->flip) flip = SDL_FLIP_HORIZONTAL;
    if (options->alpha > 0.0f)
      alpha = (Uint8)(clampf(options->alpha, 0.0f, 1.0f) * 255.0f);
    if (options->r || options->g || options->b) {
      r = options->r;
      g = options->g;
      b = options->b;
    }
  }
  SDL_SetTextureAlphaMod(texture.handle, alpha);
  SDL_SetTextureColorMod(texture.handle, r, g, b);
  SDL_RenderCopyExF(gfx->renderer, texture.handle, src ? &source : NULL,
                    &target, angle, NULL, flip);
}

void gfx_rect(gfx_t* gfx, rect_t rect, uint8_t r, uint8_t g, uint8_t b,
              uint8_t a) {
  SDL_SetRenderDrawColor(gfx->renderer, r, g, b, a);
  SDL_FRect target = {rect.x, rect.y, rect.w, rect.h};
  SDL_RenderFillRectF(gfx->renderer, &target);
}

void gfx_text(gfx_t* gfx, float x, float y, const char* text, float scale,
              uint8_t r, uint8_t g, uint8_t b) {
  if (!gfx->font) return;
  SDL_SetTextureColorMod(gfx->font, r, g, b);
  SDL_SetTextureAlphaMod(gfx->font, 255);
  float cursor = x;
  for (const char* p = text; *p; p++) {
    int index = (unsigned char)*p - GLYPH_FIRST;
    if (*p >= 'a' && *p <= 'z') index -= 32;
    if (index >= 0 && index < GLYPH_COUNT && *p != ' ') {
      SDL_Rect source = {(index % GLYPH_COLUMNS) * GLYPH_CELL_W,
                         (index / GLYPH_COLUMNS) * GLYPH_CELL_H,
                         FONT5X7_WIDTH, FONT5X7_HEIGHT};
      SDL_FRect target = {cursor, y, FONT5X7_WIDTH * scale,
                          FONT5X7_HEIGHT * scale};
      SDL_RenderCopyF(gfx->renderer, gfx->font, &source, &target);
    }
    cursor += GLYPH_CELL_W * scale;
  }
}

float gfx_text_width(const char* text, float scale) {
  return (float)strlen(text) * GLYPH_CELL_W * scale - scale;
}
