/**
 * @file scenery.h
 * @brief Procedural pixel art for the backdrop: sky, three layers of
 * mountains and forest that tile sideways, clouds, trees, bushes, the
 * castle and the glow of the spell. Pure pixel buffers, no SDL; the
 * renderer turns them into textures.
 *
 * Pixels are 0xAABBGGRR (bytes R, G, B, A in memory).
 */
#ifndef GAME_RENDER_SCENERY_H_
#define GAME_RENDER_SCENERY_H_

#include <stdbool.h>
#include <stdint.h>

#include "level.h"

typedef struct {
  int w, h;
  uint32_t* pixels;
} image_t;

bool image_create(image_t* image, int w, int h);
void image_free(image_t* image);
uint32_t image_at(const image_t* image, int x, int y);

#define PIXEL(r, g, b, a) \
  ((uint32_t)(r) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24))

#define MOUNTAIN_LAYERS 3
/* Width and height in art pixels of every layer (each art pixel is
 * SCENERY_PIXEL world units). */
#define LAYER_W 512
#define LAYER_H 136
#define SCENERY_PIXEL 4

/* A 1 x h strip, deep blue at the top to pale at the horizon. */
bool scenery_sky(image_t* image, int h);

/* How one run looks: a time of day tinting the sky, the mountains and the
 * clouds (multiplied into their colours), and where along the backdrop the
 * run starts. The same seed always gives the same look. */
#define SCENERY_THEMES 5
typedef struct {
  int kind; /* 0 day, 1 sunset, 2 dusk, 3 dawn, 4 overcast */
  uint8_t sky[3];
  uint8_t layer[MOUNTAIN_LAYERS][3];
  uint8_t cloud[3];
  float scroll; /* 0..1 of the way along the mountain tile */
} scenery_theme_t;
scenery_theme_t scenery_theme(uint32_t seed);
/* The ridge line of a layer at column x (periodic in LAYER_W): the row of
 * the first mountain pixel. Layer 0 is far and snowy, 2 is the near forest. */
int scenery_ridge(int layer, int x);
/* A LAYER_W x LAYER_H layer, transparent above the ridge, that tiles. */
bool scenery_layer(image_t* image, int layer);
bool scenery_cloud(image_t* image, uint32_t seed);
bool scenery_tree(image_t* image, decor_kind_t kind, uint32_t seed);
bool scenery_castle(image_t* image);
bool scenery_glow(image_t* image);

#endif  // GAME_RENDER_SCENERY_H_
