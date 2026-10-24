#include "demo.h"

#include <math.h>
#include <string.h>

#define GROUND_Y ((float)(GROUND_ROW * TILE))
#define PLATFORM_ROW (GROUND_ROW - 4)

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
  memset(demo, 0, sizeof(*demo));
  demo->potion_col = 58;
  demo->pistol_col = 104;
  int cols[2] = {demo->potion_col, demo->pistol_col};
  item_kind_t what[2] = {ITEM_POTION, ITEM_PISTOL};
  for (int i = 0; i < 2; i++) {
    level->tiles[PLATFORM_ROW][cols[i] - 1] = TILE_BRICK;
    level->tiles[PLATFORM_ROW][cols[i]] = TILE_QBLOCK;
    level->content[PLATFORM_ROW][cols[i]] = (uint8_t)what[i];
    level->tiles[PLATFORM_ROW][cols[i] + 1] = TILE_BRICK;
  }
  level->goal_x = level_width() - 100.0f;
  world->spawning = false;
  world->camera_x = 0.0f;
}

bool demo_finished(const demo_t* demo) { return demo->phase == DEMO_DONE; }

static void enter(demo_t* d, demo_phase_t phase) {
  d->phase = phase;
  d->phase_time = 0.0f;
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

static void spawn_ahead(world_t* world, bool bug, int variant, float dx, float height) {
  float x = world->player.x + dx;
  if (!bug) spawn_zombie(world, x, GROUND_Y);
  else spawn_bug(world, variant, x, GROUND_Y - height);
}

/* Walks at an enemy and jumps on it. With `wait_for_hit`, he first walks
 * into it and is knocked back. */
static bool encounter(world_t* world, demo_t* d, buttons_t* b, bool bug,
                      int variant, bool wait_for_hit) {
  const player_t* p = &world->player;
  if (!d->entered) {
    d->entered = true;
    d->base_zombies = world->zombies_killed;
    d->base_bugs = world->bugs_killed;
    d->base_hp = p->hp;
    spawn_ahead(world, bug, variant, 430.0f, 0.0f);
  }
  int killed = bug ? world->bugs_killed - d->base_bugs
                   : world->zombies_killed - d->base_zombies;
  if (killed > 0) {
    d->stage_time += 1.0f / 60.0f;
    return d->stage_time > 1.0f;
  }
  if (p->hp < d->base_hp || p->hurt_timer > 0.0f) d->hurt_seen = true;
  const enemy_t* e = nearest_enemy(world);
  if (!e) return d->phase_time > 20.0f;

  if (wait_for_hit && !d->hurt_seen) {
    b->right = true;
    return false;
  }
  if (p->grounded) {
    float dx = e->x - p->x;
    if (p->hurt_timer <= 0.0f && fabsf(dx) < 200.0f) {
      try_jump(d, 0.75f);
    }
    if (fabsf(dx) > 90.0f && dx > 0.0f && !wait_for_hit) b->right = true;
    if (wait_for_hit && dx > 120.0f) b->right = true;
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

/* Jumps from the ground up onto the 3-tile platform, then walks to the item
 * that came out of the block. */
static bool climb_for_item(world_t* world, demo_t* d, buttons_t* b, int col,
                           bool have) {
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
  float dx, height;
} spawn_t;

static const spawn_t kSpellWaves[] = {
    {0.3f, false, 0, 460.0f, 0.0f},  {1.0f, true, BUG_FLY, 540.0f, 40.0f},
    {2.4f, false, 0, 480.0f, 0.0f},  {2.6f, true, BUG_BEETLE, 540.0f, 0.0f},
    {4.2f, true, BUG_BEE, 580.0f, 50.0f}, {4.4f, false, 0, 500.0f, 0.0f},
    {6.0f, false, 0, 470.0f, 0.0f},  {6.3f, true, BUG_FLY, 520.0f, 30.0f},
    {7.4f, true, BUG_SMALL, 470.0f, 0.0f},
};

static const spawn_t kGunWaves[] = {
    {0.3f, false, 0, 460.0f, 0.0f},  {1.2f, true, BUG_FLY, 540.0f, 40.0f},
    {2.5f, false, 0, 480.0f, 0.0f},  {3.0f, true, BUG_BEETLE, 540.0f, 0.0f},
    {4.6f, true, BUG_BEE, 580.0f, 50.0f}, {5.0f, false, 0, 500.0f, 0.0f},
    {6.8f, false, 0, 470.0f, 0.0f},  {7.2f, true, BUG_SMALL, 520.0f, 0.0f},
};

static bool enemy_ahead(const world_t* world, float reach) {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    const enemy_t* e = &world->enemies[i];
    float dx = (e->x - world->player.x);
    if (e->alive && dx > 0.0f && dx < reach) return true;
  }
  return false;
}

/* Comes down off the platform, then fights the scheduled waves by shooting
 * from the spot. */
static bool fight(world_t* world, demo_t* d, buttons_t* b, const spawn_t* waves,
                  int count, float duration) {
  const player_t* p = &world->player;
  if (d->stage == 0) {
    b->right = true;
    if (p->grounded && p->y > GROUND_Y - 4.0f) {
      d->stage = 1;
      d->stage_time = 0.0f;
    }
    return false;
  }
  d->stage_time += 1.0f / 60.0f;
  while (d->wave < count && d->stage_time >= waves[d->wave].time) {
    const spawn_t* w = &waves[d->wave++];
    spawn_ahead(world, w->bug, w->variant, w->dx, w->height);
  }
  if (enemy_ahead(world, 600.0f)) b->shoot = true;
  const enemy_t* near = nearest_enemy(world);
  if (near && fabsf(near->x - p->x) < 44.0f) b->melee = true;
  return d->wave >= count && d->stage_time > duration && enemies_alive(world) == 0;
}

buttons_t demo_decide(world_t* world, demo_t* d, float dt) {
  buttons_t b;
  memset(&b, 0, sizeof(b));
  player_t* p = &world->player;
  d->phase_time += dt;
  d->jump_cool -= dt;
  if (p->alive && p->hp < 2) p->hp = 2; /* the showcase never ends in tears */

  bool next = false;
  switch (d->phase) {
    case DEMO_WALK:
      b.right = true;
      next = d->phase_time > 2.6f;
      break;
    case DEMO_STOMP_ZOMBIE:
      next = encounter(world, d, &b, false, 0, false);
      break;
    case DEMO_PUSHED_BY_ZOMBIE:
      next = encounter(world, d, &b, false, 0, true);
      break;
    case DEMO_STOMP_BUG:
      next = encounter(world, d, &b, true, BUG_BEETLE, false);
      break;
    case DEMO_PUSHED_BY_BUG:
      next = encounter(world, d, &b, true, BUG_SMALL, true);
      break;
    case DEMO_TO_POTION_BLOCK:
      next = under_block(world, d, &b, d->potion_col);
      break;
    case DEMO_CLIMB_FOR_POTION:
      next = climb_for_item(world, d, &b, d->potion_col, p->potions > 0);
      break;
    case DEMO_DRINK:
      if (p->grounded && !d->drink_sent) try_jump(d, 0.8f);
      if (!p->grounded && p->vy > -200.0f && !d->drink_sent) {
        b.drink = true;
        d->drink_sent = true;
      }
      next = d->drink_sent && p->grounded && p->spell_time > 0.0f;
      break;
    case DEMO_SPELL_FIGHT:
      next = fight(world, d, &b, kSpellWaves, 9, 9.0f) || p->spell_time <= 0.0f;
      break;
    case DEMO_TO_PISTOL_BLOCK:
      b.right = false;
      next = under_block(world, d, &b, d->pistol_col);
      break;
    case DEMO_CLIMB_FOR_PISTOL:
      next = climb_for_item(world, d, &b, d->pistol_col, p->has_gun);
      break;
    case DEMO_GUN_FIGHT:
      next = fight(world, d, &b, kGunWaves, 8, 8.5f);
      break;
    case DEMO_FINALE:
      b.right = true;
      if (p->grounded && fmodf(d->phase_time, 1.2f) < 0.05f) try_jump(d, 0.7f);
      next = d->phase_time > 3.5f;
      break;
    case DEMO_DONE:
      break;
  }
  if (next && d->phase != DEMO_DONE) enter(d, (demo_phase_t)(d->phase + 1));

  if (d->jump_hold > 0.0f) {
    b.jump = true;
    d->jump_hold -= dt;
    if (d->jump_hold <= 0.0f) d->jump_cool = 0.2f;
  }
  return b;
}

const char* demo_caption(const demo_t* demo) {
  switch (demo->phase) {
    case DEMO_WALK: return "FABIO TAKES A WALK";
    case DEMO_STOMP_ZOMBIE: return "JUMP ON THE ZOMBIE";
    case DEMO_PUSHED_BY_ZOMBIE: return "ANOTHER ZOMBIE PUSHES HIM BACK... THEN BOOM";
    case DEMO_STOMP_BUG: return "SAME TRICK ON THE BUGS";
    case DEMO_PUSHED_BY_BUG: return "THE BUG BITES BACK... NOT FOR LONG";
    case DEMO_TO_POTION_BLOCK: return "A ? BLOCK AHEAD";
    case DEMO_CLIMB_FOR_POTION: return "THE TDD POTION!";
    case DEMO_DRINK: return "GLUG GLUG GLUG";
    case DEMO_SPELL_FIGHT: return "THE TDD SPELL";
    case DEMO_TO_PISTOL_BLOCK: return "ANOTHER ? BLOCK";
    case DEMO_CLIMB_FOR_PISTOL: return "A GUN!";
    case DEMO_GUN_FIGHT: return "PEW PEW PEW";
    case DEMO_FINALE: return "ZOMBIES DON'T TEST. FABIO DOES.";
    case DEMO_DONE: return "THE END";
  }
  return "";
}
