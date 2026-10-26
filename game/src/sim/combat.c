#include <math.h>

#include "world.h"

#define MUZZLE_HEIGHT 30.0f
#define MUZZLE_REACH 26.0f
#define MELEE_REACH 54.0f
#define STOMP_BOUNCE 470.0f

void world_add_effect(world_t* world, fx_kind_t kind, float x, float y,
                      float duration, int facing) {
  effect_t* fx = pool_acquire(&world->effects, NULL);
  if (fx) *fx = (effect_t){kind, x, y, 0.0f, duration, facing};
}

void world_award(world_t* world, int points, float x, float y) {
  world->score += points;
  popup_t* popup = pool_acquire(&world->popups, NULL);
  if (popup) *popup = (popup_t){x, y, 0.0f, points};
}

void world_burst(world_t* world, float x, float y, int count, uint8_t r,
                 uint8_t g, uint8_t b, float speed) {
  for (int n = 0; n < count; n++) {
    particle_t* particle = pool_acquire(&world->particles, NULL);
    if (!particle) break;
    float angle = rng_float(&world->rng) * 6.2832f;
    float power = speed * (0.3f + rng_float(&world->rng) * 0.7f);
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
  }
}

void world_kill_enemy(world_t* world, enemy_t* e, bool by_spell) {
  if (!pool_is_active(&world->enemies, pool_index_of(&world->enemies, e)))
    return;
  pool_release_object(&world->enemies, e);
  float cx = e->x, cy = e->y - e->h * 0.5f;
  if (e->kind == ENEMY_BOOK_READER) {
    /* He laughs until he bursts, in a shower of pages. */
    world_add_effect(world, FX_BOOK_READER_BOOM, cx, e->y, 1.7f, e->facing);
    world_burst(world, cx, cy, 16, 235, 225, 190, 300.0f);
    world_burst(world, cx, cy, 10, 120, 70, 30, 260.0f);
    world_award(world, 500, cx, e->y - e->h);
    world->book_readers_killed++;
    world_emit(world, EVENT_BOOK_READER_DIES, cx);
  } else if (e->kind == ENEMY_ZOMBIE) {
    world_add_effect(world, by_spell ? FX_ZOMBIE_GREEN : FX_ZOMBIE_BOOM, cx,
                     e->y, 0.55f, e->facing);
    world_burst(world, cx, cy, 14, 150, 30, 30, 260.0f);
    world_burst(world, cx, cy, 8, by_spell ? 140 : 90, by_spell ? 230 : 120,
                by_spell ? 80 : 70, 220.0f);
    world_award(world, 200, cx, e->y - e->h);
    world->zombies_killed++;
    world_emit(world, EVENT_ZOMBIE_DIES, cx);
  } else {
    world_add_effect(world, by_spell ? FX_BUG_BLUE : FX_BUG_BOOM, cx, e->y,
                     0.45f, e->facing);
    world_burst(world, cx, cy, 10, 90, 60, 30, 240.0f);
    world_burst(world, cx, cy, 6, by_spell ? 80 : 255, by_spell ? 170 : 160,
                by_spell ? 255 : 40, 200.0f);
    world_award(world, 100, cx, e->y - e->h);
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
  bullet_t* bullet = pool_acquire(&world->bullets, NULL);
  if (!bullet) return;
  float x, y;
  muzzle(world, &x, &y);
  *bullet = (bullet_t){x, y, (float)p->facing * BULLET_SPEED, 1.1f};
  world_add_effect(world, FX_MUZZLE, x, y, 0.07f, p->facing);
  p->shoot_pose = 0.2f;
  world_emit(world, EVENT_SHOOT, x);
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
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (frect_overlaps(beam, enemy_rect(e))) world_kill_enemy(world, e, true);
  }
  ray_t* ray = pool_acquire(&world->rays, NULL);
  if (ray) *ray = (ray_t){x0, x1, y, 0.0f, p->facing};
  world_burst(world, x1, y, 6, 110, 200, 255, 160.0f);
  p->cast_pose = 0.28f;
  world_emit(world, EVENT_SPELL, x0);
}

static void update_bullets(world_t* world, float dt) {
  size_t cursor = 0;
  for (bullet_t* b; (b = pool_next_active(&world->bullets, &cursor));) {
    b->x += b->vx * dt;
    b->life -= dt;
    if (b->life <= 0.0f || level_solid_at(&world->level, b->x, b->y)) {
      world_burst(world, b->x, b->y, 4, 250, 210, 90, 120.0f);
      pool_release_object(&world->bullets, b);
      continue;
    }
    frect_t box = {b->x - 6.0f, b->y - 12.0f, 12.0f, 24.0f};
    size_t target_cursor = 0;
    for (enemy_t* enemy;
         (enemy = pool_next_active(&world->enemies, &target_cursor));) {
      if (frect_overlaps(box, enemy_rect(enemy))) {
        world_kill_enemy(world, enemy, false);
        pool_release_object(&world->bullets, b);
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
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (e->kind == ENEMY_BOOK_READER) continue;
    if (frect_overlaps(reach, enemy_rect(e))) {
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
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (e->kind == ENEMY_BOOK_READER) continue;
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

  size_t cursor = 0;
  for (ray_t* ray; (ray = pool_next_active(&world->rays, &cursor));) {
    ray->age += dt;
    if (ray->age > 0.22f) pool_release_object(&world->rays, ray);
  }
  cursor = 0;
  for (effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));) {
    fx->age += dt;
    if (fx->age >= fx->duration) pool_release_object(&world->effects, fx);
  }
  cursor = 0;
  for (popup_t* popup; (popup = pool_next_active(&world->popups, &cursor));) {
    popup->age += dt;
    popup->y -= POPUP_RISE / POPUP_SECONDS * dt;
    if (popup->age >= POPUP_SECONDS) pool_release_object(&world->popups, popup);
  }
  cursor = 0;
  for (particle_t* particle;
       (particle = pool_next_active(&world->particles, &cursor));) {
    particle->age += dt;
    particle->vy += 900.0f * dt;
    particle->x += particle->vx * dt;
    particle->y += particle->vy * dt;
    if (particle->age >= particle->life)
      pool_release_object(&world->particles, particle);
  }
}
