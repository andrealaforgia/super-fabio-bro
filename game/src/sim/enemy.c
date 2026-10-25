#include <math.h>
#include <stddef.h>

#include "world.h"

#define ZOMBIE_W 24.0f
#define ZOMBIE_H 54.0f
#define SPAWN_MARGIN 40.0f

static const struct {
  float w, h, speed;
  bool flies;
} kBugs[BUG_VARIANTS] = {
    {30.0f, 26.0f, 70.0f, false}, /* beetle */
    {28.0f, 24.0f, 85.0f, true},  /* fly */
    {26.0f, 24.0f, 110.0f, true}, /* bee */
    {32.0f, 34.0f, 55.0f, false}, /* dark beetle: hops */
    {22.0f, 18.0f, 120.0f, false} /* small beetle: quick */
};

frect_t enemy_rect(const enemy_t* e) {
  frect_t r = {e->x - e->w * 0.5f, e->y - e->h, e->w, e->h};
  return r;
}

static enemy_t* free_enemy(world_t* world) {
  for (int i = 0; i < MAX_ENEMIES; i++)
    if (!world->enemies[i].alive) return &world->enemies[i];
  return NULL;
}

void spawn_zombie(world_t* world, float x, float y) {
  enemy_t* e = free_enemy(world);
  if (!e) return;
  *e = (enemy_t){0};
  e->alive = true;
  e->kind = ENEMY_ZOMBIE;
  e->x = x;
  e->y = y;
  e->w = ZOMBIE_W;
  e->h = ZOMBIE_H;
  e->facing = world->player.x < x ? -1 : 1;
  float progress = world_progress(world);
  e->speed = 42.0f + rng_float(&world->rng) * 26.0f + progress * 22.0f;
  e->anim_time = rng_float(&world->rng) * 3.0f;
  e->hop_cooldown = 0.5f;
  e->phase_seed = rng_float(&world->rng) * 6.28f;
  world_emit(world, EVENT_GROAN, x);
}

void spawn_bug(world_t* world, int variant, float x, float y) {
  enemy_t* e = free_enemy(world);
  if (!e || variant < 0 || variant >= BUG_VARIANTS) return;
  *e = (enemy_t){0};
  e->alive = true;
  e->kind = ENEMY_BUG;
  e->variant = variant;
  e->x = x;
  e->y = y;
  e->w = kBugs[variant].w;
  e->h = kBugs[variant].h;
  e->speed = kBugs[variant].speed * (0.85f + rng_float(&world->rng) * 0.3f);
  e->facing = world->player.x < x ? -1 : 1;
  e->phase = rng_float(&world->rng) * 6.28f;
  e->base_y = y;
  e->hop_cooldown = 1.0f;
}

static bool is_flier(const enemy_t* e) {
  return e->kind == ENEMY_BUG && kBugs[e->variant].flies;
}

static void walk_towards_player(world_t* world, enemy_t* e, float dt) {
  const player_t* p = &world->player;
  float dx = p->x - e->x;
  float direction = fabsf(dx) < 3.0f ? 0.0f : (dx > 0.0f ? 1.0f : -1.0f);
  if (direction != 0.0f) e->facing = (int)direction;
  e->vx = direction * e->speed;
  e->vy = fminf(e->vy + GRAVITY * dt, MAX_FALL);
  e->hop_cooldown -= dt;
  move_result_t moved = physics_move(&world->level, &e->x, &e->y, &e->vx,
                                     &e->vy, e->w, e->h, dt);
  e->grounded = moved.grounded;
  bool blocked = moved.hit_left || moved.hit_right;
  bool hopper = e->kind == ENEMY_BUG && e->variant == BUG_DARK;
  if (e->grounded && e->hop_cooldown <= 0.0f && (blocked || hopper)) {
    e->vy = blocked ? -560.0f : -430.0f;
    e->hop_cooldown = hopper ? 1.3f : 0.9f;
    e->grounded = false;
  }
}

static void fly_towards_player(world_t* world, enemy_t* e, float dt) {
  const player_t* p = &world->player;
  e->phase += dt * (e->variant == BUG_BEE ? 7.0f : 4.5f);
  float dx = p->x - e->x;
  if (fabsf(dx) > 3.0f) e->facing = dx > 0.0f ? 1 : -1;
  e->x += (dx > 0.0f ? 1.0f : -1.0f) * e->speed * dt;
  float target = p->y - 40.0f + sinf(e->phase) * 34.0f;
  e->y += (target - e->y) * fminf(1.0f, 1.6f * dt);
  e->y = clampf(e->y, 48.0f, level_height() - 8.0f);
}

void enemies_update(world_t* world, float dt) {
  const player_t* p = &world->player;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_t* e = &world->enemies[i];
    if (!e->alive) continue;
    e->anim_time += dt;
    if (is_flier(e))
      fly_towards_player(world, e, dt);
    else
      walk_towards_player(world, e, dt);
    bool fell = e->y > level_height() + 120.0f;
    bool left_behind = fabsf(e->x - p->x) > 2400.0f;
    if (fell || left_behind) e->alive = false;
  }
}

/* The surface to stand on in column `col`, nearest the player's row. */
static bool ground_in_column(const world_t* world, int col, float* y) {
  int player_row = (int)(world->player.y / TILE);
  for (int row = player_row - 6; row <= player_row + 4; row++) {
    if (row < 1) continue;
    if (tile_is_solid(level_tile(&world->level, col, row)) &&
        !tile_is_solid(level_tile(&world->level, col, row - 1)) &&
        !tile_is_solid(level_tile(&world->level, col, row - 2))) {
      *y = (float)(row * TILE);
      return true;
    }
  }
  return false;
}

static bool pick_spawn_point(world_t* world, bool ground, float* x, float* y) {
  for (int attempt = 0; attempt < 6; attempt++) {
    bool right = rng_chance(&world->rng, 0.72f);
    float edge = right ? world->camera_x + world->view_w + SPAWN_MARGIN
                       : world->camera_x - SPAWN_MARGIN;
    float px = edge + (right ? 1.0f : -1.0f) * rng_float(&world->rng) * 90.0f;
    if (px < TILE || px > level_width() - TILE) continue;
    if (!ground) {
      *x = px;
      *y = clampf(world->player.y - 60.0f - rng_float(&world->rng) * 110.0f,
                  70.0f, level_height() - 60.0f);
      return true;
    }
    float surface;
    if (ground_in_column(world, (int)(px / TILE), &surface)) {
      *x = px;
      *y = surface;
      return true;
    }
  }
  return false;
}

void spawner_update(world_t* world, float dt) {
  if (!world->spawning) return;
  float progress = world_progress(world);
  world->zombie_timer -= dt;
  world->bug_timer -= dt;

  if (world->zombie_timer <= 0.0f) {
    int limit = 5 + (int)(progress * 6.0f);
    float x, y;
    if (world_count_enemies(world, ENEMY_ZOMBIE) < limit &&
        pick_spawn_point(world, true, &x, &y)) {
      spawn_zombie(world, x, y);
      world->zombie_timer = lerpf(4.2f, 1.7f, progress) +
                            rng_float(&world->rng) * 1.2f;
    } else {
      world->zombie_timer = 0.4f;
    }
  }

  if (world->bug_timer <= 0.0f) {
    int limit = 3 + (int)(progress * 4.0f);
    int variant = rng_range(&world->rng, 0, BUG_VARIANTS - 1);
    float x, y;
    if (world_count_enemies(world, ENEMY_BUG) < limit &&
        pick_spawn_point(world, !kBugs[variant].flies, &x, &y)) {
      spawn_bug(world, variant, x, y);
      world->bug_timer = lerpf(6.5f, 3.2f, progress) +
                         rng_float(&world->rng) * 2.0f;
    } else {
      world->bug_timer = 0.5f;
    }
  }
}
