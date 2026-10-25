/**
 * @file sdl_gfx.h
 * @brief The 2D drawing surface: an SDL window and renderer, textures from
 * pixels or files, and text in the built-in 5x7 font.
 *
 * Drawing is in world units. The view is always `GFX_VIEW_HEIGHT` units tall
 * and as wide as the window's shape allows, so the world fills the window at
 * any size.
 */
#ifndef ENGINE_RENDERER_SDL_GFX_H_
#define ENGINE_RENDERER_SDL_GFX_H_

#include <stdbool.h>
#include <stdint.h>

#include "geometry.h"

#define GFX_VIEW_HEIGHT 544.0f

struct SDL_Texture;

typedef struct gfx gfx_t;

typedef struct {
  struct SDL_Texture* handle;
  int w, h;
} texture_t;

typedef struct {
  bool flip;       /* mirror left to right */
  float alpha;     /* 0..1; 0 is treated as 1 */
  float angle;     /* degrees about the centre */
  uint8_t r, g, b; /* colour multiplier; all 0 is treated as white */
} draw_t;

/* Opens a resizable window as large as the desktop allows. */
gfx_t* gfx_open(const char* title, int width, int height);
void gfx_close(gfx_t* gfx);

void gfx_begin(gfx_t* gfx, uint8_t r, uint8_t g, uint8_t b);
void gfx_end(gfx_t* gfx);
float gfx_view_width(const gfx_t* gfx);
float gfx_scale(const gfx_t* gfx);
void gfx_toggle_fullscreen(gfx_t* gfx);
/* Saves the frame drawn so far (call before gfx_end). */
bool gfx_save_bmp(gfx_t* gfx, const char* path);

/* Pixels are 0xAABBGGRR in memory order R, G, B, A. */
texture_t gfx_texture_from_pixels(gfx_t* gfx, const uint32_t* pixels, int w,
                                  int h, bool smooth, bool additive);
texture_t gfx_texture_from_file(gfx_t* gfx, const char* path, bool smooth);
void gfx_texture_free(texture_t* texture);

/* `src` may be NULL for the whole texture. */
void gfx_draw(gfx_t* gfx, texture_t texture, const frect_t* src, frect_t dst,
              const draw_t* options);
void gfx_rect(gfx_t* gfx, frect_t rect, uint8_t r, uint8_t g, uint8_t b,
              uint8_t a);
void gfx_text(gfx_t* gfx, float x, float y, const char* text, float scale,
              uint8_t r, uint8_t g, uint8_t b);
float gfx_text_width(const char* text, float scale);

#endif  // ENGINE_RENDERER_SDL_GFX_H_
