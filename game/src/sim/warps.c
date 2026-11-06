#include <math.h>
#include <string.h>

#include "world.h"

#define PIPE_EDGE 6.0f /* how far in from the sides of the cap he has to stand */

int warp_pipe_under_fabio(const world_t* world) {
  const player_t* p = &world->player;
  if (!p->alive || !p->grounded || p->flying || p->platform >= 0 || world_drumming(world)) return -1;
  const level_t* level = &world->level;
  for (int i = 0; i < level->warp_count; i++) {
    if (!level->cave && world->warp_used[i]) continue;
    float left = (float)(level->warp_col[i] * TILE) + PIPE_EDGE;
    float right = (float)((level->warp_col[i] + 2) * TILE) - PIPE_EDGE;
    float cap = (float)(level->warp_row[i] * TILE);
    if (p->x >= left && p->x <= right && fabsf(p->y - cap) < 2.0f) return i;
  }
  return -1;
}

float warp_fade(const world_t* world) {
  const warp_t* w = &world->warp;
  if (w->phase == WARP_SINKING) {
    float start = WARP_SINK_SECONDS - 0.25f;
    return w->t > start ? clampf((w->t - start) / 0.25f, 0.0f, 1.0f) : 0.0f;
  }
  if (w->phase == WARP_RISING) return clampf(1.0f - w->t / 0.3f, 0.0f, 1.0f);
  return 0.0f;
}

/* Down into the cave under the pipe: the main level is put aside. */
static void go_down(world_t* world, int pipe) {
  warp_stash_t* s = &world->stash;
  s->level = world->level;
  memcpy(s->item_taken, world->item_taken, sizeof(s->item_taken));
  s->helicopter = world->helicopter;
  memcpy(s->drums, world->drums, sizeof(s->drums));
  s->drum_count = world->drum_count;
  s->camera_x = world->camera_x;
  s->spawning = world->spawning;
  world->warp_from = pipe;
  world->warp_used[pipe] = true;

  level_generate_cave(&world->level, world->seed, pipe);
  memset(world->item_taken, 0, sizeof(world->item_taken));
  world->helicopter = (helicopter_t){false, 0.0f, 0.0f};
  world->drum_count = 0;
  world->spawning = false;
  world_clear_actors(world);
  platforms_reset(world);
  player_t* p = &world->player;
  p->x = (float)((world->level.warp_col[0] + 1) * TILE);
  p->y = (float)(world->level.warp_row[0] * TILE);
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  p->safe_x = p->x;
  p->safe_y = p->y;
  world->camera_x = 0.0f;
  allies_regroup(world);
  float ground = (float)(GROUND_ROW * TILE);
  if (world->level.guardian == 1) spawn_gorilla(world, world->level.guardian_x, ground);
  else if (world->level.guardian == 2) spawn_slop(world, world->level.guardian_x, ground);
}

/* Back up to the main level, out of the pipe he went down. */
static void go_up(world_t* world) {
  warp_stash_t* s = &world->stash;
  world->level = s->level;
  memcpy(world->item_taken, s->item_taken, sizeof(world->item_taken));
  world->helicopter = s->helicopter;
  memcpy(world->drums, s->drums, sizeof(world->drums));
  world->drum_count = s->drum_count;
  world->camera_x = s->camera_x;
  world->spawning = s->spawning;
  world_clear_actors(world);
  platforms_reset(world);
  player_t* p = &world->player;
  int pipe = world->warp_from;
  p->x = (float)((world->level.warp_col[pipe] + 1) * TILE);
  p->y = (float)(world->level.warp_row[pipe] * TILE);
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  p->safe_x = p->x;
  p->safe_y = p->y;
  allies_regroup(world);
}

void warps_update(world_t* world, const controls_t* controls, float dt) {
  warp_t* w = &world->warp;
  player_t* p = &world->player;
  if (w->phase == WARP_NONE) {
    if (world->state != STATE_PLAYING || !controls_pressed(controls, BTN_DOWN)) return;
    int pipe = warp_pipe_under_fabio(world);
    if (pipe < 0) return;
    *w = (warp_t){WARP_SINKING, 0.0f, pipe};
    p->vx = 0.0f;
    world_emit(world, EVENT_WARP, p->x);
    return;
  }
  w->t += dt;
  if (w->phase == WARP_SINKING && w->t >= WARP_SINK_SECONDS) {
    if (world->level.cave) go_up(world);
    else go_down(world, w->pipe);
    *w = (warp_t){WARP_RISING, 0.0f, w->pipe};
    world_emit(world, EVENT_WARP, p->x);
  } else if (w->phase == WARP_RISING && w->t >= WARP_RISE_SECONDS) {
    *w = (warp_t){WARP_NONE, 0.0f, 0};
    p->invulnerable = fmaxf(p->invulnerable, 0.6f);
  }
}
