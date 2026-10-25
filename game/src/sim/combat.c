#include <math.h>

#include "world.h"

#define MUZZLE_HEIGHT 30.0f
#define MUZZLE_REACH 26.0f
#define MELEE_REACH 54.0f
#define STOMP_BOUNCE 470.0f

void world_add_effect(world_t* world, fx_kind_t kind, float x, float y,
                      float duration, int facing) {
  for (int i = 0; i < MAX_EFFECTS; i++) {
    effect_t* fx = &world->effects[i];
    if (fx->alive) continue;
    *fx = (effect_t){true, kind, x, y, 0.0f, duration, facing};
    return;
  }
}

void world_burst(world_t* world, float x, float y, int count, uint8_t r,
                 uint8_t g, uint8_t b, float speed) {
  for (int n = 0; n < count; n++) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
      particle_t* particle = &world->particles[i];
      if (particle->alive) continue;
      float angle = rng_float(&world->rng) * 6.2832f;
      float power = speed * (0.3f + rng_float(&world->rng) * 0.7f);
      particle->alive = true;
      particle->x = x;
      particle->y = y;
      particle->vx = cosf(angle) * power;
      particle->vy = sinf(angle) * power - speed * 0.4f;
      particle->age = 0.0f;
      particle->life = 0.4f + rng_float(&world->rng) * 0.5f;
      particle->size = 2.0f + rng_float(&world->rng) * 4.0f;
      particle->r = r;
      particle->g = g;
      particle->b = b;
      break;
    }
  }
}

void world_kill_enemy(world_t* world, enemy_t* e, bool by_spell) {
  if (!e->alive) return;
  e->alive = false;
  float cx = e->x, cy = e->y - e->h * 0.5f;
  if (e->kind == ENEMY_ZOMBIE) {
    world_add_effect(world, by_spell ? FX_ZOMBIE_GREEN : FX_ZOMBIE_BOOM, cx,
                     e->y, 0.55f, e->facing);
    world_burst(world, cx, cy, 14, 150, 30, 30, 260.0f);
    world_burst(world, cx, cy, 8, by_spell ? 140 : 90, by_spell ? 230 : 120,
                by_spell ? 80 : 70, 220.0f);
    world->score += 200;
    world->zombies_killed++;
    world_emit(world, EVENT_ZOMBIE_DIES, cx);
  } else {
    world_add_effect(world, by_spell ? FX_BUG_BLUE : FX_BUG_BOOM, cx, e->y,
                     0.45f, e->facing);
    world_burst(world, cx, cy, 10, 90, 60, 30, 240.0f);
    world_burst(world, cx, cy, 6, by_spell ? 80 : 255, by_spell ? 170 : 160,
                by_spell ? 255 : 40, 200.0f);
    world->score += 100;
    world->bugs_killed++;
    world_emit(world, EVENT_BUG_DIES, cx);
  }
}

static void muzzle(const world_t* world, float* x, float* y) {
  const player_t* p = &world->player;
  *x = p->x + (float)p->facing * MUZZLE_REACH;
  *y = p->y - MUZZLE_HEIGHT;
}

void world_fire_bullet(world_t* world) {
  player_t* p = &world->player;
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullet_t* bullet = &world->bullets[i];
    if (bullet->alive) continue;
    float x, y;
    muzzle(world, &x, &y);
    *bullet = (bullet_t){true, x, y, (float)p->facing * BULLET_SPEED, 1.1f};
    world_add_effect(world, FX_MUZZLE, x, y, 0.07f, p->facing);
    p->shoot_pose = 0.2f;
    world_emit(world, EVENT_SHOOT, x);
    return;
  }
}

void world_cast_spell(world_t* world) {
  player_t* p = &world->player;
  float x0, y;
  muzzle(world, &x0, &y);
  float x1 = x0;
  for (float d = 0.0f; d < SPELL_LENGTH; d += 8.0f) {
    float x = x0 + (float)p->facing * d;
    if (level_solid_at(&world->level, x, y)) break;
    x1 = x;
  }
  float left = fminf(x0, x1), right = fmaxf(x0, x1);
  frect_t beam = {left, y - 16.0f, right - left, 32.0f};
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_t* e = &world->enemies[i];
    if (e->alive && frect_overlaps(beam, enemy_rect(e)))
      world_kill_enemy(world, e, true);
  }
  for (int i = 0; i < MAX_RAYS; i++) {
    if (world->rays[i].alive) continue;
    world->rays[i] = (ray_t){x0, x1, y, 0.0f, p->facing, true};
    break;
  }
  world_burst(world, x1, y, 6, 110, 200, 255, 160.0f);
  p->cast_pose = 0.28f;
  world_emit(world, EVENT_SPELL, x0);
}

static void update_bullets(world_t* world, float dt) {
  for (int i = 0; i < MAX_BULLETS; i++) {
    bullet_t* b = &world->bullets[i];
    if (!b->alive) continue;
    b->x += b->vx * dt;
    b->life -= dt;
    if (b->life <= 0.0f || level_solid_at(&world->level, b->x, b->y)) {
      world_burst(world, b->x, b->y, 4, 250, 210, 90, 120.0f);
      b->alive = false;
      continue;
    }
    frect_t box = {b->x - 6.0f, b->y - 12.0f, 12.0f, 24.0f};
    for (int e = 0; e < MAX_ENEMIES; e++) {
      enemy_t* enemy = &world->enemies[e];
      if (enemy->alive && frect_overlaps(box, enemy_rect(enemy))) {
        world_kill_enemy(world, enemy, false);
        b->alive = false;
        break;
      }
    }
  }
}

static void update_melee(world_t* world) {
  player_t* p = &world->player;
  if (p->melee_timer <= 0.0f || p->melee_timer > 0.26f ||
      p->melee_timer < 0.06f)
    return;
  float x0 = p->facing > 0 ? p->x : p->x - MELEE_REACH;
  frect_t reach = {x0 - 4.0f, p->y - PLAYER_H - 6.0f, MELEE_REACH + 8.0f,
                  PLAYER_H + 6.0f};
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_t* e = &world->enemies[i];
    if (e->alive && frect_overlaps(reach, enemy_rect(e))) {
      world_kill_enemy(world, e, false);
      p->melee_struck = true;
    }
  }
}

static frect_t inset(frect_t r, float by) {
  frect_t out = {r.x + by, r.y + by, r.w - 2.0f * by, r.h - 2.0f * by};
  return out;
}

static void touch_enemies(world_t* world) {
  player_t* p = &world->player;
  if (!p->alive) return;
  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemy_t* e = &world->enemies[i];
    if (!e->alive) continue;
    if (!frect_overlaps(inset(player_rect(p), 3.0f), inset(enemy_rect(e), 3.0f)))
      continue;
    /* Landing on the top part counts; low bugs get a little extra leeway. */
    float stomp_line = fmaxf(e->y - e->h * 0.45f, e->y - e->h + 16.0f);
    bool from_above = p->vy > 60.0f && p->y < stomp_line;
    if (from_above) {
      world_kill_enemy(world, e, false);
      p->vy = -STOMP_BOUNCE;
      p->grounded = false;
      world_emit(world, EVENT_STOMP, p->x);
    } else {
      player_hurt(world, e->x);
    }
  }
}

void combat_update(world_t* world, float dt) {
  update_bullets(world, dt);
  update_melee(world);
  touch_enemies(world);

  for (int i = 0; i < MAX_RAYS; i++) {
    ray_t* ray = &world->rays[i];
    if (!ray->alive) continue;
    ray->age += dt;
    if (ray->age > 0.22f) ray->alive = false;
  }
  for (int i = 0; i < MAX_EFFECTS; i++) {
    effect_t* fx = &world->effects[i];
    if (!fx->alive) continue;
    fx->age += dt;
    if (fx->age >= fx->duration) fx->alive = false;
  }
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particle_t* particle = &world->particles[i];
    if (!particle->alive) continue;
    particle->age += dt;
    particle->vy += 900.0f * dt;
    particle->x += particle->vx * dt;
    particle->y += particle->vy * dt;
    if (particle->age >= particle->life) particle->alive = false;
  }
}
