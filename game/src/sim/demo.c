#include "demo.h"

#include <math.h>
#include <string.h>

#define GROUND_Y ((float)(GROUND_ROW * TILE))
#define PLATFORM_ROW (GROUND_ROW - 4)
#define EDGE_MARGIN 36.0f

/* The course, left to right, as a script. */
static const demo_step_t kScript[] = {
    {STEP_TRAVEL, 8},
    {STEP_STOMP_ZOMBIE, 0},
    {STEP_TRAVEL, 70},
    {STEP_PUSHED_BY_ZOMBIE, 0},
    {STEP_TRAVEL, 120},
    {STEP_STOMP_BUG, 0},
    {STEP_TRAVEL, 174},
    {STEP_PUSHED_BY_BUG, 0},
    {STEP_BLOCK, 212},
    {STEP_CLIMB, 212},
    {STEP_DRINK, 0},
    {STEP_SPELL_FIGHT, 0},
    {STEP_BLOCK, 262},
    {STEP_CLIMB, 262},
    {STEP_DRINK, 0},
    {STEP_SPELL_FIGHT, 0},
    {STEP_BLOCK, 320},
    {STEP_CLIMB, 320},
    {STEP_DRINK, 0},
    {STEP_SPELL_FIGHT, 0},
    {STEP_BLOCK, 384},
    {STEP_CLIMB, 384},
    {STEP_GUN_FIGHT, 0},
    {STEP_CASTLE, 0},
};

int demo_step_count(void) { return (int)(sizeof(kScript) / sizeof(kScript[0])); }
const demo_step_t* demo_steps(void) { return kScript; }

/* ---- building the course ---- */

static void put(level_t* level, int col, int row, tile_t tile) {
  if (col >= 0 && col < LEVEL_COLS && row >= 0 && row < LEVEL_ROWS)
    level->tiles[row][col] = (uint8_t)tile;
}

static void coin(level_t* level, int col, int row) {
  if (level->item_count >= MAX_LEVEL_ITEMS) return;
  placement_t* item = &level->items[level->item_count++];
  item->kind = ITEM_COIN;
  item->x = (float)(col * TILE + TILE / 2);
  item->y = (float)(row * TILE + TILE / 2);
}

static void pit(level_t* level, int col, int width) {
  for (int i = 0; i < width; i++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++) put(level, col + i, row, TILE_EMPTY);
}

static void pipe(level_t* level, int col, int height) {
  for (int h = 1; h <= height; h++)
    for (int dx = 0; dx < 2; dx++)
      put(level, col + dx, GROUND_ROW - h,
          h == height ? (dx ? TILE_PIPE_TOP_R : TILE_PIPE_TOP_L)
                      : (dx ? TILE_PIPE_BODY_R : TILE_PIPE_BODY_L));
  coin(level, col, GROUND_ROW - height - 1);
  coin(level, col + 1, GROUND_ROW - height - 1);
}

static void stairs(level_t* level, int col, int steps) {
  for (int i = 0; i < steps; i++)
    for (int h = 1; h <= i + 1; h++) put(level, col + i, GROUND_ROW - h, TILE_STONE);
  for (int h = 1; h <= steps; h++) put(level, col + steps, GROUND_ROW - h, TILE_STONE);
  for (int i = 0; i < steps; i++)
    for (int h = 1; h <= steps - 1 - i; h++)
      put(level, col + steps + 1 + i, GROUND_ROW - h, TILE_STONE);
}

static void stepping_stones(level_t* level, int col, int width) {
  pit(level, col, width);
  for (int i = 1; i < width - 1; i += 2) {
    put(level, col + i, GROUND_ROW - 3, TILE_GOLD);
    coin(level, col + i, GROUND_ROW - 4);
  }
}

static void bricks(level_t* level, int col, int length, int row) {
  for (int i = 0; i < length; i++) {
    put(level, col + i, row, (i % 3 == 1) ? TILE_QBLOCK : TILE_BRICK);
    if ((i % 3 == 1)) level->content[row][col + i] = ITEM_COIN;
    coin(level, col + i, row - 1);
  }
}

static void item_platform(level_t* level, int col, item_kind_t what) {
  put(level, col - 1, PLATFORM_ROW, TILE_BRICK);
  put(level, col, PLATFORM_ROW, TILE_QBLOCK);
  level->content[PLATFORM_ROW][col] = (uint8_t)what;
  put(level, col + 1, PLATFORM_ROW, TILE_BRICK);
}

static void scenery(level_t* level, uint32_t seed) {
  rng_t rng;
  rng_seed(&rng, seed);
  level->decor_count = 0;
  for (int col = 4; col < 460 && level->decor_count < MAX_LEVEL_DECOR; col += rng_range(&rng, 4, 9)) {
    if (!tile_is_solid(level_tile(level, col, GROUND_ROW))) continue;
    if (level_tile(level, col, GROUND_ROW - 1) != TILE_EMPTY) continue;
    int roll = rng_range(&rng, 0, 9);
    decor_t* d = &level->decor[level->decor_count++];
    d->kind = roll < 5 ? DECOR_PINE : (roll < 8 ? DECOR_OAK : DECOR_BUSH);
    d->x = (float)(col * TILE + TILE / 2);
    d->y = GROUND_Y;
  }
}

void demo_world_init(world_t* world, demo_t* demo, uint32_t seed) {
  world_init(world, seed);
  level_t* level = &world->level;
  memset(level->tiles, 0, sizeof(level->tiles));
  memset(level->content, 0, sizeof(level->content));
  level->item_count = 0;
  for (int col = 0; col < LEVEL_COLS; col++) {
    level->tiles[GROUND_ROW][col] = TILE_GRASS;
    for (int row = GROUND_ROW + 1; row < LEVEL_ROWS; row++)
      level->tiles[row][col] = TILE_ROCK;
  }
  /* Open ground for each fight, obstacles in between. */
  for (int c = 8; c < 34; c += 5) coin(level, c, GROUND_ROW - 2);
  pipe(level, 38, 2);
  bricks(level, 44, 6, PLATFORM_ROW);
  pit(level, 54, 3);
  stairs(level, 60, 3);
  pipe(level, 100, 2);
  stepping_stones(level, 106, 5);
  pipe(level, 116, 2);
  pit(level, 150, 3);
  stairs(level, 156, 3);
  bricks(level, 166, 6, PLATFORM_ROW);
  pipe(level, 242, 2);
  pit(level, 247, 3);
  pipe(level, 253, 2);
  stepping_stones(level, 292, 5);
  pipe(level, 300, 2);
  stairs(level, 304, 3);
  pit(level, 350, 3);
  pipe(level, 356, 2);
  bricks(level, 360, 6, PLATFORM_ROW);
  stepping_stones(level, 369, 5);
  /* Three potions and the pistol. */
  item_platform(level, 212, ITEM_POTION);
  item_platform(level, 262, ITEM_POTION);
  item_platform(level, 320, ITEM_POTION);
  item_platform(level, 384, ITEM_PISTOL);
  /* The castle, up a flight of stairs. */
  stairs(level, 428, 4);
  level->castle_x = (float)(450 * TILE);
  level->goal_x = level->castle_x;
  scenery(level, seed);

  memset(demo, 0, sizeof(*demo));
  world->spawning = false;
  world->camera_x = 0.0f;
}

bool demo_finished(const demo_t* demo) { return demo->finished_time > 3.5f; }

/* ---- the director ---- */

static void enter(demo_t* d, int step) {
  d->step = step;
  d->step_time = 0.0f;
  d->entered = false;
  d->stage = 0;
  d->stage_time = 0.0f;
  d->wave = 0;
  d->hurt_seen = false;
  d->drink_sent = false;
}

static void try_jump(demo_t* d, float hold) {
  if (d->jump_hold <= 0.0f && d->jump_cool <= 0.0f) d->jump_hold = hold;
}

static const enemy_t* nearest_enemy(const world_t* world) {
  const enemy_t* best = NULL;
  float best_distance = 1e9f;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    const enemy_t* e = &world->enemies[i];
    if (!e->alive) continue;
    float distance = fabsf(e->x - world->player.x);
    if (distance < best_distance) {
      best = e;
      best_distance = distance;
    }
  }
  return best;
}

static int enemies_alive(const world_t* world) {
  return world_count_enemies(world, ENEMY_ZOMBIE) + world_count_enemies(world, ENEMY_BUG);
}

static void steer(buttons_t* b, const player_t* p, float target_x) {
  if (target_x > p->x + 5.0f) b->right = true;
  else if (target_x < p->x - 5.0f) b->left = true;
}

/* Enemies walk in from just beyond the right edge of the screen. */
static void spawn_from_the_right(world_t* world, bool bug, int variant,
                                 float jitter, float height) {
  float x = world->camera_x + world->view_w + EDGE_MARGIN + jitter;
  if (x > level_width() - TILE) x = level_width() - TILE;
  /* Never inside a stair or a pipe. */
  bool flier = bug && (variant == BUG_FLY || variant == BUG_BEE);
  if (!flier && level_solid_at(&world->level, x, GROUND_Y - 8.0f)) return;
  if (!bug) {
    spawn_zombie(world, x, GROUND_Y);
  } else {
    spawn_bug(world, variant, x, GROUND_Y - height);
  }
  /* A brisk shuffle keeps the show moving. */
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_t* e = &world->enemies[i];
    if (e->alive && e->x >= x - 0.01f && e->x <= x + 0.01f && e->kind == ENEMY_ZOMBIE)
      e->speed = 78.0f + jitter * 0.1f;
  }
}

static bool encounter(world_t* world, demo_t* d, buttons_t* b, bool bug,
                      int variant, bool wait_for_hit) {
  const player_t* p = &world->player;
  if (!d->entered) {
    d->entered = true;
    d->base_zombies = world->zombies_killed;
    d->base_bugs = world->bugs_killed;
    d->base_hp = p->hp;
    spawn_from_the_right(world, bug, variant, 0.0f, 0.0f);
  }
  int killed = bug ? world->bugs_killed - d->base_bugs
                   : world->zombies_killed - d->base_zombies;
  if (killed > 0) {
    d->stage_time += 1.0f / 60.0f;
    return d->stage_time > 1.0f;
  }
  if (p->hp < d->base_hp || p->hurt_timer > 0.0f) d->hurt_seen = true;
  const enemy_t* e = nearest_enemy(world);
  if (!e) return d->step_time > 25.0f;

  if (wait_for_hit && !d->hurt_seen) {
    b->right = true;
    return false;
  }
  if (p->grounded) {
    float dx = e->x - p->x;
    if (p->hurt_timer <= 0.0f && fabsf(dx) < 200.0f) try_jump(d, 0.75f);
    if (dx > (wait_for_hit ? 120.0f : 90.0f)) b->right = true;
  } else {
    steer(b, p, e->x);
  }
  return false;
}

static bool under_block(const world_t* world, demo_t* d, buttons_t* b, int col) {
  const player_t* p = &world->player;
  float centre = (float)(col * TILE + TILE / 2);
  if (world->level.tiles[PLATFORM_ROW][col] == TILE_USED) return true;
  float dx = centre - p->x;
  if (fabsf(dx) > 5.0f) {
    if (dx > 0.0f) b->right = true;
    else b->left = true;
  } else if (p->grounded && fabsf(p->vx) < 30.0f) {
    try_jump(d, 0.7f);
  }
  return false;
}

static bool climb_for_item(world_t* world, demo_t* d, buttons_t* b, int col, bool have) {
  const player_t* p = &world->player;
  float left = (float)((col - 1) * TILE);
  bool on_platform = p->grounded && p->y < GROUND_Y - 100.0f;
  if (on_platform) {
    if (have) return true;
    for (int i = 0; i < MAX_PICKUPS; i++)
      if (world->pickups[i].alive) steer(b, p, world->pickups[i].x);
    return false;
  }
  if (!p->grounded) {
    b->right = true;
    return false;
  }
  float start = left - 62.0f;
  float dx = start - p->x;
  if (fabsf(dx) > 3.0f) {
    if (dx > 0.0f) b->right = true;
    else b->left = true;
  } else if (fabsf(p->vx) < 25.0f) {
    try_jump(d, 0.8f);
    b->right = true;
  }
  return false;
}

typedef struct {
  float time;
  bool bug;
  int variant;
  float jitter, height;
} spawn_t;

static const spawn_t kWaves[] = {
    {0.3f, false, 0, 0.0f, 0.0f},   {1.4f, true, BUG_FLY, 40.0f, 40.0f},
    {2.8f, false, 0, 20.0f, 0.0f},  {3.0f, true, BUG_BEETLE, 70.0f, 0.0f},
    {4.8f, true, BUG_BEE, 30.0f, 50.0f}, {5.0f, false, 0, 60.0f, 0.0f},
    {6.6f, false, 0, 0.0f, 0.0f},   {6.9f, true, BUG_SMALL, 50.0f, 0.0f},
};
#define WAVE_COUNT ((int)(sizeof(kWaves) / sizeof(kWaves[0])))

static bool enemy_ahead(const world_t* world, float reach) {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    const enemy_t* e = &world->enemies[i];
    float dx = e->x - world->player.x;
    if (e->alive && dx > 0.0f && dx < reach) return true;
  }
  return false;
}

/* Comes down off the platform, then fights the scheduled waves, advancing
 * on them while shooting. */
static bool fight(world_t* world, demo_t* d, buttons_t* b, float duration) {
  const player_t* p = &world->player;
  if (d->stage == 0) {
    b->right = true;
    if (p->grounded && p->y > GROUND_Y - 4.0f) {
      d->stage = 1;
      d->stage_x = p->x;
      d->stage_time = 0.0f;
    }
    return false;
  }
  d->stage_time += 1.0f / 60.0f;
  while (d->wave < WAVE_COUNT && d->stage_time >= kWaves[d->wave].time) {
    const spawn_t* w = &kWaves[d->wave++];
    spawn_from_the_right(world, w->bug, w->variant, w->jitter, w->height);
  }
  /* Let them come into view before the shooting starts. */
  if (enemy_ahead(world, 430.0f)) b->shoot = true;
  const enemy_t* near = nearest_enemy(world);
  if (near && near->x > p->x + 160.0f && !enemy_ahead(world, 430.0f) &&
      p->x < d->stage_x + 6.0f * TILE)
    b->right = true;
  if (near && fabsf(near->x - p->x) < 44.0f) b->melee = true;
  return d->wave >= WAVE_COUNT && d->stage_time > duration && enemies_alive(world) == 0;
}

const char* demo_caption(const demo_t* demo) {
  if (demo->finished_time > 0.0f) return "THE END. ZOMBIES DON'T TEST. FABIO DOES.";
  switch (kScript[demo->step].kind) {
    case STEP_TRAVEL: return "FABIO ON THE MOVE";
    case STEP_STOMP_ZOMBIE: return "JUMP ON THE ZOMBIE";
    case STEP_PUSHED_BY_ZOMBIE: return "ANOTHER ZOMBIE PUSHES HIM BACK... THEN BOOM";
    case STEP_STOMP_BUG: return "SAME TRICK ON THE BUGS";
    case STEP_PUSHED_BY_BUG: return "THE BUG BITES BACK... NOT FOR LONG";
    case STEP_BLOCK: return "A ? BLOCK AHEAD";
    case STEP_CLIMB: return "THE TDD POTION!";
    case STEP_DRINK: return "GLUG GLUG GLUG";
    case STEP_SPELL_FIGHT: return "THE TDD SPELL";
    case STEP_GUN_FIGHT: return "A GUN! PEW PEW PEW";
    case STEP_CASTLE: return "TO THE CASTLE";
  }
  return "";
}

buttons_t demo_decide(world_t* world, demo_t* d, float dt) {
  buttons_t b;
  memset(&b, 0, sizeof(b));
  player_t* p = &world->player;
  if (world->state == STATE_WON) {
    d->finished_time += dt;
    return b;
  }
  d->step_time += dt;
  d->jump_cool -= dt;
  if (p->alive && p->hp < 2) p->hp = 2; /* the showcase never ends in tears */

  const demo_step_t* step = &kScript[d->step];
  bool next = false;
  switch (step->kind) {
    case STEP_TRAVEL:
      bot_travel(world, &d->travel, dt, &b, false);
      /* Hold back a moment so a landing is not rushed into the next jump. */
      next = p->x >= (float)(step->arg * TILE);
      break;
    case STEP_STOMP_ZOMBIE: next = encounter(world, d, &b, false, 0, false); break;
    case STEP_PUSHED_BY_ZOMBIE: next = encounter(world, d, &b, false, 0, true); break;
    case STEP_STOMP_BUG: next = encounter(world, d, &b, true, BUG_BEETLE, false); break;
    case STEP_PUSHED_BY_BUG: next = encounter(world, d, &b, true, BUG_SMALL, true); break;
    case STEP_BLOCK:
      if (p->x < (float)((step->arg - 3) * TILE)) {
        bot_travel(world, &d->travel, dt, &b, false);
      } else {
        next = under_block(world, d, &b, step->arg);
      }
      break;
    case STEP_CLIMB:
      next = climb_for_item(world, d, &b, step->arg,
                            kScript[d->step - 1].kind == STEP_BLOCK &&
                                    world->level.content[PLATFORM_ROW][step->arg] == ITEM_PISTOL
                                ? p->has_gun
                                : p->potions > 0);
      break;
    case STEP_DRINK:
      if (p->grounded && !d->drink_sent) try_jump(d, 0.8f);
      if (!p->grounded && p->vy > -200.0f && !d->drink_sent) {
        b.drink = true;
        d->drink_sent = true;
      }
      next = d->drink_sent && p->grounded && p->spell_time > 0.0f;
      break;
    case STEP_SPELL_FIGHT:
      next = fight(world, d, &b, 8.0f) || p->spell_time <= 0.0f;
      break;
    case STEP_GUN_FIGHT:
      next = fight(world, d, &b, 8.0f);
      break;
    case STEP_CASTLE:
      bot_travel(world, &d->travel, dt, &b, false);
      break;
  }
  if (next && d->step + 1 < demo_step_count()) enter(d, d->step + 1);

  if (d->jump_hold > 0.0f) {
    b.jump = true;
    d->jump_hold -= dt;
    if (d->jump_hold <= 0.0f) d->jump_cool = 0.2f;
  }
  return b;
}
