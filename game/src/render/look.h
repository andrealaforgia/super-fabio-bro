/**
 * @file look.h
 * @brief Everything the renderer draws with, loaded once: the cooked
 * sprites as textures and the procedural scenery turned into textures.
 */
#ifndef GAME_RENDER_LOOK_H_
#define GAME_RENDER_LOOK_H_

#include "pose.h"
#include "scenery.h"
#include "sdl_gfx.h"

#define CLOUD_VARIANTS 3

typedef struct {
  texture_t texture;
  int w, h, anchor_x, anchor_y;
} sprite_t;

typedef struct {
  sprite_t* frames[STRIP_COUNT];
  texture_t sky;
  texture_t layers[MOUNTAIN_LAYERS];
  texture_t clouds[CLOUD_VARIANTS];
  texture_t trees[3]; /* indexed by decor_kind_t */
  texture_t castle;
  texture_t glow;
  texture_t cover;
} look_t;

/* `sprite_dir` holds manifest.txt and the PNGs. Returns false and prints
 * what was missing. */
bool look_load(look_t* look, gfx_t* gfx, const char* sprite_dir);
void look_free(look_t* look);
const sprite_t* look_sprite(const look_t* look, pose_t pose);

#endif  // GAME_RENDER_LOOK_H_
