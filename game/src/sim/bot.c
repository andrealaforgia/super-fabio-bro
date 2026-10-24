#include "bot.h"

#include <math.h>

/* Columns of missing ground ahead of the player. */
static int pit_width(const level_t* level, float x, float y) {
  int width = 0;
  for (float probe = x + 14.0f; width < 8; probe += TILE) {
    bool floor = false;
    for (float depth = 4.0f; depth <= 100.0f; depth += 32.0f)
      floor = floor || level_solid_at(level, probe, y + depth);
    if (floor) break;
    width++;
  }
  return width;
}

static bool enemy_ahead(const world_t* world, float reach, float height) {
  const player_t* p = &world->player;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    const enemy_t* e = &world->enemies[i];
    if (!e->alive) continue;
    float dx = (e->x - p->x) * (float)p->facing;
    if (dx > 0.0f && dx < reach && fabsf((e->y - e->h * 0.5f) - (p->y - 30.0f)) < height)
      return true;
  }
  return false;
}

/* Just the movement: heads right, jumping walls and pits. */
static void bot_travel(const world_t* world, bot_t* bot, float dt, buttons_t* b,
                bool run_always) {
  const player_t* p = &world->player;
  const level_t* level = &world->level;
  b->right = true;
  b->run = run_always;

  float ahead = p->x + 22.0f;
  bool wall = level_solid_at(level, ahead, p->y - 6.0f) ||
              level_solid_at(level, ahead, p->y - 30.0f);
  float edge = p->x + 14.0f;
  bool pit = !level_solid_at(level, edge, p->y + 4.0f) &&
             !level_solid_at(level, edge, p->y + 36.0f) &&
             !level_solid_at(level, edge, p->y + 68.0f);
  int width = pit ? pit_width(level, p->x, p->y) : 0;
  if (width >= 4) b->run = true;
  if (p->grounded && (wall || pit)) {
    /* A fresh press needs the button released first. A narrow pit wants a
     * short hop so he lands before the next one. */
    bot->jump_hold = bot->jump_hold > 0.0f ? 0.0f : (pit && width <= 3 && b->run ? 0.14f : 0.75f);
  } else if (p->grounded) {
    bot->jump_hold = 0.0f;
  }
  if (bot->jump_hold > 0.0f) {
    b->jump = true;
    bot->jump_hold -= dt;
  }
  if (!p->grounded && bot->jump_hold > 0.0f && width >= 4) b->run = true;
}

buttons_t bot_decide(const world_t* world, bot_t* bot, float dt) {
  const player_t* p = &world->player;
  buttons_t b = {0};
  bot_travel(world, bot, dt, &b, true);

  bool armed = p->has_gun || p->spell_time > 0.0f;
  if (armed && enemy_ahead(world, 460.0f, 70.0f)) b.shoot = true;
  if (enemy_ahead(world, 44.0f, 70.0f)) b.melee = true;

  bot->drink_wait -= dt;
  if (p->potions > 0 && p->spell_time <= 0.0f && bot->drink_wait <= 0.0f &&
      enemy_ahead(world, 360.0f, 120.0f)) {
    b.drink = true;
    bot->drink_wait = 0.5f;
  }
  return b;
}
