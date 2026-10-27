/**
 * @file pose.h
 * @brief Which sprite to draw for a character right now. Every sprite the
 * game uses is named here, as a strip of frames (count 0 is a single image
 * named by its prefix). Pure, so poses can be tested without a window.
 */
#ifndef GAME_RENDER_POSE_H_
#define GAME_RENDER_POSE_H_

#include "world.h"

typedef enum {
  STRIP_FAB_IDLE,
  STRIP_FAB_WALK,
  STRIP_FAB_RUN,
  STRIP_FAB_JUMP,
  STRIP_FAB_ATTACK,
  STRIP_FAB_HURT,
  STRIP_FAB_GUNWALK,
  STRIP_FAB_GUNAIM,
  STRIP_FAB_GUNSHOOT,
  STRIP_FAB_POTION,
  STRIP_FAB_DRINK,
  STRIP_FAB_SPELL,
  STRIP_FAB_CAST,
  STRIP_ZOMBIE,
  STRIP_ZOMBIE_ATTACK,
  STRIP_BUG_BEETLE,
  STRIP_BUG_FLY,
  STRIP_BUG_BEE,
  STRIP_BUG_DARK,
  STRIP_BUG_SMALL,
  STRIP_FX_MUZZLE,
  STRIP_FX_BULLET,
  STRIP_FX_RAY,
  STRIP_FX_BUG_SPLAT,
  STRIP_FX_BUG_BOOM,
  STRIP_FX_BUG_BLUE,
  STRIP_FX_ZOMBIE_GREEN,
  STRIP_FX_ZOMBIE_BOOM,
  STRIP_FX_SMOKE,
  STRIP_FX_REMAINS,
  STRIP_PISTOL,
  STRIP_BOTTLE,
  STRIP_COIN,
  STRIP_MUSHROOM,
  STRIP_TILE_SOLID,
  STRIP_TILE_Q,
  STRIP_TILE_BRICK,
  STRIP_TILE_GOLD,
  STRIP_TILE_STONE,
  STRIP_GROUND_GRASS,
  STRIP_GROUND_DIRT,
  STRIP_GROUND_ROCK,
  STRIP_PIPE_CAP,
  STRIP_PIPE_BODY,
  STRIP_BUBBLE,
  STRIP_READER_READ,
  STRIP_READER_LAUGH,
  STRIP_READER_POP,
  STRIP_READER_FLASH,
  STRIP_READER_BOOM,
  STRIP_READER_REMAINS,
  STRIP_MONK,
  STRIP_MONK_BOOM,
  STRIP_SKELETON,
  STRIP_SKELETON_VANISH,
  STRIP_BUBBLE_MONK,
  STRIP_BUBBLE_SKELETON,
  STRIP_COUNT
} strip_id_t;

typedef struct {
  const char* prefix;
  int frames; /* 0: a single image named `prefix` */
} strip_spec_t;

extern const strip_spec_t kStripSpecs[STRIP_COUNT];

typedef struct {
  strip_id_t strip;
  int frame;
} pose_t;

pose_t pose_player(const player_t* player);
pose_t pose_zombie(const enemy_t* zombie, const player_t* player);
pose_t pose_bug(const enemy_t* bug);
pose_t pose_effect(fx_kind_t kind);
pose_t pose_book_reader(const enemy_t* book_reader);
/* The book reader bursting: he swells, flashes, booms and leaves smoking
 * remains. `age` is seconds since the shot. */
pose_t pose_book_reader_boom(float age);
/* The monk goes up in an explosion that thins to smoke and then a little
 * debris. `age` is seconds since the hit. */
pose_t pose_monk_boom(float age);
/* The skeleton dissolves into dust fast: a frame every 45 milliseconds. */
pose_t pose_skeleton_vanish(float age);

#endif  // GAME_RENDER_POSE_H_
