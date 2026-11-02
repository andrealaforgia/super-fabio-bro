#include <math.h>
#include <stddef.h>

#include "world.h"

#define ZOMBIE_W 24.0f
#define ZOMBIE_H 54.0f
#define SPAWN_MARGIN 40.0f
#define BOOK_READER_W 26.0f
#define BOOK_READER_H 54.0f
#define MONK_W 29.0f
#define MONK_H 65.0f
#define SKELETON_W 18.0f
#define SKELETON_H 39.0f
#define CHEF_W 26.0f
#define CHEF_H 58.0f

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

void spawn_zombie(world_t* world, float x, float y) {
  enemy_t* e = pool_acquire(&world->enemies, NULL);
  if (!e) return;
  e->kind = ENEMY_ZOMBIE;
  e->leap = 0;
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
  if (variant < 0 || variant >= BUG_VARIANTS) return;
  enemy_t* e = pool_acquire(&world->enemies, NULL);
  if (!e) return;
  e->kind = ENEMY_BUG;
  e->leap = 0;
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

void spawn_book_reader(world_t* world, float x, float y) {
  enemy_t* e = pool_acquire(&world->enemies, NULL);
  if (!e) return;
  e->kind = ENEMY_BOOK_READER;
  e->leap = 0;
  e->x = x;
  e->y = y;
  e->w = BOOK_READER_W;
  e->h = BOOK_READER_H;
  e->facing = world->player.x < x ? -1 : 1; /* looking back at Fabio */
  e->anim_time = 0.0f; /* he starts out reading */
}

void spawn_monk(world_t* world, monk_variant_t variant, float x, float y) {
  enemy_t* e = pool_acquire(&world->enemies, NULL);
  if (!e) return;
  e->kind = ENEMY_MONK;
  e->leap = 0;
  e->variant = variant;
  e->x = x;
  e->y = y;
  e->w = variant == MONK_SKELETON ? SKELETON_W : MONK_W;
  e->h = variant == MONK_SKELETON ? SKELETON_H : MONK_H;
  e->facing = world->player.x < x ? -1 : 1;
  e->phase_seed = rng_float(&world->rng) * 6.28f; /* his speech bobs out of step */
  if (variant == MONK_SKELETON) { /* he walks, slowly, towards Fabio */
    e->speed = 34.0f + rng_float(&world->rng) * 14.0f + world_progress(world) * 14.0f;
    e->hop_cooldown = 0.5f;
  }
}

void spawn_chef(world_t* world, float x, float y) {
  enemy_t* e = pool_acquire(&world->enemies, NULL);
  if (!e) return;
  *e = (enemy_t){0};
  e->kind = ENEMY_CHEF;
  e->x = x;
  e->y = y;
  e->w = CHEF_W;
  e->h = CHEF_H;
  e->facing = world->player.x < x ? -1 : 1;
}

/* For the chef, `phase` is the time spent swinging the knife: zero while Fabio is far. */
bool chef_attacking(const enemy_t* c) {
  return c->phase > 0.0f;
}

int chef_swing_frame(const enemy_t* c) {
  return (int)(c->phase * CHEF_SWING_FPS) % CHEF_SWING_FRAMES;
}

bool chef_knife_out(const enemy_t* c) {
  return chef_attacking(c) && chef_swing_frame(c) >= CHEF_SWING_FRAMES / 2;
}

bool chef_has_room(const world_t* world, float x, float y) {
  int first = (int)((x - CHEF_W) / TILE);
  int last = (int)((x + CHEF_W) / TILE);
  int row = (int)(y / TILE);
  if (first < 0 || last >= LEVEL_COLS || row < CHEF_HEIGHT_TILES) return false;
  for (int col = first; col <= last; col++) {
    if (level_tile(&world->level, col, row) != TILE_GRASS) return false;
    for (int above = 1; above <= CHEF_HEIGHT_TILES; above++)
      if (tile_is_solid(level_tile(&world->level, col, row - above))) return false;
  }
  return true;
}

static void chef_watch_fabio(enemy_t* c, const player_t* p, float dt) {
  bool near = fabsf(p->x - c->x) < CHEF_ATTACK_RANGE && fabsf(p->y - c->y) < 80.0f;
  c->phase = near ? c->phase + dt : 0.0f;
}

book_reader_mood_t book_reader_mood(const enemy_t* m) {
  float in_cycle = fmodf(m->anim_time, BOOK_READER_READ_SECONDS + BOOK_READER_LAUGH_SECONDS);
  return in_cycle < BOOK_READER_READ_SECONDS ? BOOK_READER_READING : BOOK_READER_LAUGHING;
}

static bool is_flier(const enemy_t* e) {
  return e->kind == ENEMY_BUG && kBugs[e->variant].flies;
}

/* The top of the ledge in column `col` that a climber out of a pit can reach: the
 * ground, or a block, stairs or pipe standing on it, up to PIT_EXIT_HEIGHT tiles
 * high, with room above to stand. Returns false when there is no such ledge. */
static bool ledge_top(const world_t* world, int col, float* top) {
  int row = GROUND_ROW;
  if (!tile_is_solid(level_tile(&world->level, col, row))) return false;
  while (row > GROUND_ROW - PIT_EXIT_HEIGHT && tile_is_solid(level_tile(&world->level, col, row - 1)))
    row--;
  if (tile_is_solid(level_tile(&world->level, col, row - 1)) ||
      tile_is_solid(level_tile(&world->level, col, row - 2)))
    return false;
  *top = (float)(row * TILE);
  return true;
}

/* Which way out of the pit under `x`: -1 left, 1 right, 0 when there is no ledge
 * near enough on either side. The side `prefer` wins when it is in reach,
 * otherwise the nearer one. `top` and `column` describe the ledge it picked. */
static int pit_exit_side(const world_t* world, float x, int prefer, float* top, int* column) {
  int col = (int)(x / TILE);
  float unused_top;
  int unused_column;
  if (!top) top = &unused_top;
  if (!column) column = &unused_column;
  for (int pass = 0; pass < 2; pass++) {
    int side = pass == 0 ? prefer : -prefer;
    for (int step = 0; step <= PIT_EXIT_REACH; step++) {
      if (ledge_top(world, col + side * step, top)) {
        *column = col + side * step;
        return side;
      }
    }
  }
  return 0;
}

/* Fell into a pit: jump for the edge, high enough to clear it. */
static void leap_out_of_pit(const world_t* world, enemy_t* e) {
  float top = 0.0f;
  e->leap = pit_exit_side(world, e->x, world->player.x < e->x ? -1 : 1, &top, NULL);
  if (e->leap == 0) return;
  e->vy = -sqrtf(2.0f * GRAVITY * ((e->y - top) + 44.0f));
  e->facing = e->leap;
}

/* The book reader, the monk and the chef never walk: they stand where they appeared
 * and only fall until there is ground under them. One that lands in a pit still
 * leaps out of it, and then stands where it comes down. */
static void stand_in_place(world_t* world, enemy_t* e, float dt) {
  e->vy = fminf(e->vy + GRAVITY * dt, MAX_FALL);
  if (e->y > PIT_LEAP_LINE && e->vy > 0.0f) leap_out_of_pit(world, e);
  e->vx = (float)e->leap * PIT_LEAP_SPEED;
  move_result_t moved = physics_move(&world->level, &e->x, &e->y, &e->vx,
                                     &e->vy, e->w, e->h, dt);
  e->grounded = moved.grounded;
  if (e->grounded) e->leap = 0;
}



static void walk_towards_player(world_t* world, enemy_t* e, float dt) {
  const player_t* p = &world->player;
  float dx = p->x - e->x;
  float direction = fabsf(dx) < 3.0f ? 0.0f : (dx > 0.0f ? 1.0f : -1.0f);
  if (direction != 0.0f) e->facing = (int)direction;
  e->vy = fminf(e->vy + GRAVITY * dt, MAX_FALL);
  if (e->y > PIT_LEAP_LINE && e->vy > 0.0f) leap_out_of_pit(world, e);
  /* Climbing out, it keeps pushing for the edge until it is back on its feet. */
  e->vx = e->leap != 0 ? (float)e->leap * PIT_LEAP_SPEED : direction * e->speed;
  e->hop_cooldown -= dt;
  move_result_t moved = physics_move(&world->level, &e->x, &e->y, &e->vx,
                                     &e->vy, e->w, e->h, dt);
  e->grounded = moved.grounded;
  if (e->grounded) e->leap = 0;
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
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    e->anim_time += dt;
    if (e->kind == ENEMY_MONK || e->kind == ENEMY_CHEF) e->facing = p->x < e->x ? -1 : 1; /* turns to Fabio */
    if (e->kind == ENEMY_CHEF) chef_watch_fabio(e, p, dt);
    if (e->kind == ENEMY_BOOK_READER || e->kind == ENEMY_CHEF ||
        (e->kind == ENEMY_MONK && e->variant == MONK_HOODED))
      stand_in_place(world, e, dt);
    else if (is_flier(e))
      fly_towards_player(world, e, dt);
    else
      walk_towards_player(world, e, dt);
    bool fell = e->y > level_height() + 120.0f;
    bool left_behind = fabsf(e->x - p->x) > 2400.0f;
    if (fell || left_behind) pool_release_object(&world->enemies, e);
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

/* The book readers, monks and chefs stand about where they appeared, so a new
 * one never appears on top of another character. */
static bool clear_of_characters(const world_t* world, float x) {
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (e->kind == ENEMY_ZOMBIE || e->kind == ENEMY_BUG) continue;
    if (fabsf(e->x - x) < CHARACTER_SPACING) return false;
  }
  return true;
}

/* Nobody is already down in the pit near x. */
static bool clear_of_pit_walkers(const world_t* world, float x) {
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
    if (e->y > PIT_LEAP_LINE && fabsf(e->x - x) < 2.0f * TILE) return false;
  return true;
}

/* A spot just off screen, on the right with probability `right_chance`. */
static bool pick_spawn_point(world_t* world, bool ground, float right_chance,
                             float* x, float* y) {
  for (int attempt = 0; attempt < 6; attempt++) {
    bool right = rng_chance(&world->rng, right_chance);
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

/* A spot at the bottom of a pit ahead of Fabio, from which any actor can leap out;
 * `exit_x` is the middle of the ledge it will climb onto. */
static bool pick_pit_point(world_t* world, float* x, float* y, float* exit_x) {
  int base = (int)(world->player.x / TILE);
  for (int attempt = 0; attempt < 6; attempt++) {
    int col = base + rng_range(&world->rng, 4, 14);
    if (level_tile(&world->level, col, GROUND_ROW) != TILE_EMPTY) continue;
    float px = (float)(col * TILE + TILE / 2);
    int edge;
    if (pit_exit_side(world, px, world->player.x < px ? -1 : 1, NULL, &edge) == 0 ||
        !clear_of_pit_walkers(world, px))
      continue;
    *x = px;
    *y = level_height() - 12.0f;
    *exit_x = (float)(edge * TILE + TILE / 2);
    return true;
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
    float x, y, exit_x;
    if (world_count_enemies(world, ENEMY_ZOMBIE) < limit &&
        ((rng_chance(&world->rng, PIT_AMBUSH_CHANCE) && pick_pit_point(world, &x, &y, &exit_x)) ||
         pick_spawn_point(world, true, 0.72f, &x, &y))) {
      spawn_zombie(world, x, y);
      world->zombie_timer = lerpf(4.2f, 1.7f, progress) +
                            rng_float(&world->rng) * 1.2f;
    } else {
      world->zombie_timer = 0.4f;
    }
  }

  world->monk_timer -= dt;
  if (world->monk_timer <= 0.0f) {
    float x, y, exit_x;
    monk_variant_t variant = (monk_variant_t)rng_range(&world->rng, 0, MONK_VARIANTS - 1);
    bool from_a_pit = rng_chance(&world->rng, PIT_AMBUSH_CHANCE) && pick_pit_point(world, &x, &y, &exit_x) &&
                      (variant == MONK_SKELETON || clear_of_characters(world, exit_x));
    if (world_count_enemies(world, ENEMY_MONK) < MONK_MAX &&
        (from_a_pit || (pick_spawn_point(world, true, 1.0f, &x, &y) && clear_of_characters(world, x)))) {
      spawn_monk(world, variant, x, y);
      world->monk_timer = 18.0f + rng_float(&world->rng) * 14.0f;
    } else {
      world->monk_timer = 1.0f;
    }
  }

  world->chef_timer -= dt;
  if (world->chef_timer <= 0.0f) {
    float x = 0.0f, y = 0.0f;
    float exit_x = 0.0f;
    bool found = false;
    for (int attempt = 0; attempt < 16 && !found; attempt++)
      found = pick_spawn_point(world, true, 1.0f, &x, &y) && chef_has_room(world, x, y) &&
              clear_of_characters(world, x);
    if (!found && rng_chance(&world->rng, PIT_AMBUSH_CHANCE))
      found = pick_pit_point(world, &x, &y, &exit_x) &&
              chef_has_room(world, exit_x, (float)(GROUND_ROW * TILE)) &&
              clear_of_characters(world, exit_x);
    if (world_count_enemies(world, ENEMY_CHEF) < CHEF_MAX && found) {
      spawn_chef(world, x, y);
      world->chef_timer = 40.0f + rng_float(&world->rng) * 30.0f;
    } else {
      world->chef_timer = 1.0f;
    }
  }

  world->book_reader_timer -= dt;
  if (world->book_reader_timer <= 0.0f) {
    float x, y, exit_x;
    bool from_a_pit = rng_chance(&world->rng, PIT_AMBUSH_CHANCE) &&
                      pick_pit_point(world, &x, &y, &exit_x) && clear_of_characters(world, exit_x);
    if (world_count_enemies(world, ENEMY_BOOK_READER) < BOOK_READER_MAX &&
        (from_a_pit || (pick_spawn_point(world, true, 1.0f, &x, &y) && clear_of_characters(world, x)))) {
      spawn_book_reader(world, x, y);
      world->book_reader_timer = 12.0f + rng_float(&world->rng) * 12.0f;
    } else {
      world->book_reader_timer = 1.0f;
    }
  }

  if (world->bug_timer <= 0.0f) {
    int limit = 3 + (int)(progress * 4.0f);
    int variant = rng_range(&world->rng, 0, BUG_VARIANTS - 1);
    float x, y;
    float exit_x;
    bool from_a_pit = !kBugs[variant].flies && rng_chance(&world->rng, PIT_AMBUSH_CHANCE) &&
                      pick_pit_point(world, &x, &y, &exit_x);
    if (world_count_enemies(world, ENEMY_BUG) < limit &&
        (from_a_pit || pick_spawn_point(world, !kBugs[variant].flies, 0.72f, &x, &y))) {
      spawn_bug(world, variant, x, y);
      world->bug_timer = lerpf(6.5f, 3.2f, progress) +
                         rng_float(&world->rng) * 2.0f;
    } else {
      world->bug_timer = 0.5f;
    }
  }
}
