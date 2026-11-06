#include <math.h>

#include "world.h"

#define MUZZLE_HEIGHT 30.0f
#define MUZZLE_REACH 26.0f
#define MELEE_REACH 54.0f
#define STOMP_BOUNCE 470.0f
#define BANANA_SIZE 14.4f
#define BANANA_LIFE 4.0f

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

void world_fling(world_t* world, debris_kind_t kind, float x, float y, int count) {
  for (int n = 0; n < count; n++) {
    debris_t* d = pool_acquire(&world->debris, NULL);
    if (!d) break;
    float angle = ((float)n + rng_float(&world->rng)) * 6.2832f / (float)count;
    float speed = 140.0f + rng_float(&world->rng) * 300.0f;
    bool vegetable = kind == DEBRIS_VEGETABLE;
    *d = (debris_t){kind,
                    n % (kind == DEBRIS_SLOP ? SLOP_PIECES : VEGETABLE_KINDS), /* every kind in turn */
                    x,
                    y,
                    cosf(angle) * speed,
                    sinf(angle) * speed,
                    0.0f,
                    0.8f + rng_float(&world->rng) * 0.7f,
                    vegetable ? VEGETABLE_GROWTH : 1.0f,
                    (rng_float(&world->rng) - 0.5f) * (vegetable ? 900.0f : 1800.0f)};
  }
}

/* They drift to a stop under a little gravity as they fade. */
static void update_debris(world_t* world, float dt) {
  size_t cursor = 0;
  for (debris_t* d; (d = pool_next_active(&world->debris, &cursor));) {
    d->age += dt;
    d->vy += BANANA_GRAVITY * 0.25f * dt;
    d->vx *= 1.0f - 0.8f * dt;
    d->x += d->vx * dt;
    d->y += d->vy * dt;
    if (d->age > d->life || d->y > level_height() + 60.0f) pool_release_object(&world->debris, d);
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
  } else if (e->kind == ENEMY_CHEF) {
    /* The chef bursts apart in a shower of vegetables. */
    world_add_effect(world, FX_CHEF_EXPLODE, cx, e->y, CHEF_EXPLODE_SECONDS, e->facing);
    world_fling(world, DEBRIS_VEGETABLE, cx, cy, VEGETABLE_SHOWER);
    world_burst(world, cx, cy, 16, 240, 240, 240, 320.0f);
    world_burst(world, cx, cy, 8, 200, 30, 30, 260.0f);
    world_award(world, 600, cx, e->y - e->h);
    world->chefs_killed++;
    world_emit(world, EVENT_CHEF_DIES, cx);
  } else if (e->kind == ENEMY_MONK) {
    if (e->variant == MONK_SKELETON) {
      /* He loves PRs, and fades away into dust. */
      world_add_effect(world, FX_SKELETON_VANISH, cx, e->y, 0.55f, e->facing);
      world_burst(world, cx, cy, 14, 150, 130, 190, 200.0f);
    } else {
      /* He still will not pair, and goes up in a cloud of smoke. */
      world_add_effect(world, FX_MONK_BOOM, cx, e->y, MONK_BOOM_SECONDS, e->facing);
      world_burst(world, cx, cy, 18, 255, 170, 40, 320.0f);
      world_burst(world, cx, cy, 10, 60, 50, 50, 260.0f);
    }
    world_award(world, 300, cx, e->y - e->h);
    world->monks_killed++;
    world_emit(world, EVENT_MONK_DIES, cx);
  } else if (e->kind == ENEMY_DEMON) {
    /* He clutches himself and dissolves into smoke and embers. */
    world_add_effect(world, FX_DEMON_DEATH, cx, e->y, DEMON_DEATH_SECONDS, e->facing);
    world_burst(world, cx, cy, 14, 255, 120, 30, 220.0f);
    world_award(world, 500, cx, e->y - e->h);
    world->demons_killed++;
    world_emit(world, EVENT_DEMON_DIES, cx);
  } else if (e->kind == ENEMY_SLOP) {
    /* He tears into rainbow noise and bursts into a shower of slop. */
    world_add_effect(world, FX_SLOP_DEATH, cx, e->y, 0.8f, e->facing);
    world_fling(world, DEBRIS_SLOP, cx, cy, SLOP_SHOWER);
    world_burst(world, cx, cy, 16, 200, 60, 220, 300.0f);
    world_burst(world, cx, cy, 10, 70, 220, 190, 260.0f);
    world_award(world, 450, cx, e->y - e->h);
    world->slops_killed++;
    world_emit(world, EVENT_SLOP_DIES, cx);
  } else if (e->kind == ENEMY_GORILLA) {
    /* He goes down in a cloud of fur and a shower of bananas. */
    world_add_effect(world, FX_SMOKE, cx, e->y, 0.6f, e->facing);
    world_fling(world, DEBRIS_BANANA, cx, cy, BANANA_SHOWER);
    world_burst(world, cx, cy, 20, 110, 60, 25, 300.0f);
    world_burst(world, cx, cy, 8, 230, 170, 120, 240.0f);
    world_award(world, 400, cx, e->y - e->h);
    world->gorillas_killed++;
    world_emit(world, EVENT_GORILLA_DIES, cx);
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

float skewer_tilt(const skewer_t* s) {
  if (s->vy >= 0.0f) { /* past the top: on down, stopping 10 degrees short of vertical */
    float dive = s->vy / (SKEWER_SPEED * sinf(SKEWER_ANGLE * (3.14159265f / 180.0f)));
    return -SKEWER_DIVE * fminf(dive, 1.0f);
  }
  float flight = atan2f(-s->vy, fabsf(s->vx)) * (180.0f / 3.14159265f);
  return fminf(flight, SKEWER_ANGLE);
}

void world_fire_skewer(world_t* world) {
  player_t* p = &world->player;
  skewer_t* skewer = pool_acquire(&world->skewers, NULL);
  if (!skewer) return;
  float x, y;
  muzzle(world, &x, &y);
  y = p->y - HELICOPTER_H * 0.5f;
  float rad = SKEWER_ANGLE * (3.14159265f / 180.0f);
  *skewer = (skewer_t){x, y, (float)p->facing * SKEWER_SPEED * cosf(rad),
                       -SKEWER_SPEED * sinf(rad), 0.0f};
  p->shoot_pose = 0.2f;
  world_emit(world, EVENT_SKEWER_THROWN, x);
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

/* Like a bullet: the first enemy it touches dies. Walls and the ground stop it. */
static void update_skewers(world_t* world, float dt) {
  size_t cursor = 0;
  for (skewer_t* s; (s = pool_next_active(&world->skewers, &cursor));) {
    s->age += dt;
    s->vy += SKEWER_GRAVITY * dt;
    s->x += s->vx * dt;
    s->y += s->vy * dt;
    if (s->age > SKEWER_LIFE || s->y > level_height() + 60.0f ||
        level_solid_at(&world->level, s->x, s->y)) {
      if (s->age <= SKEWER_LIFE) world_burst(world, s->x, s->y, 4, 190, 90, 60, 120.0f);
      pool_release_object(&world->skewers, s);
      continue;
    }
    frect_t box = {s->x - SKEWER_SIZE * 0.5f, s->y - SKEWER_SIZE * 0.5f, SKEWER_SIZE,
                   SKEWER_SIZE};
    size_t target_cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &target_cursor));) {
      if (frect_overlaps(box, enemy_rect(e))) {
        world_kill_enemy(world, e, false);
        pool_release_object(&world->skewers, s);
        break;
      }
    }
  }
}

/* Aimed at (tx, ty) on a low arc, longer the farther it is, so it keeps under
 * the blocks overhead. */
static void aim_banana(float x, float y, float tx, float ty, float* vx, float* vy) {
  float dx = tx - x, dy = ty - y;
  float flight = clampf(fabsf(dx) / 520.0f, 0.45f, 1.1f);
  *vx = dx / flight;
  *vy = (dy - 0.5f * BANANA_GRAVITY * flight * flight) / flight;
}

/* Whether the first moments of a throw from (x, y) at (tx, ty) are in the open. */
static bool clear_start(const world_t* world, float x, float y, float tx, float ty) {
  float vx, vy;
  aim_banana(x, y, tx, ty, &vx, &vy);
  for (float t = 0.0f; t <= 0.15f; t += 0.025f)
    if (level_solid_at(&world->level, x + vx * t, y + vy * t + 0.5f * BANANA_GRAVITY * t * t))
      return false;
  return true;
}

void world_throw_banana(world_t* world, const enemy_t* g) {
  banana_t* banana = pool_acquire(&world->bananas, NULL);
  if (!banana) return;
  /* From over his head where he swung it, so it clears a low wall; under a
   * block, from his hand in front of him; failing both, from his chest. */
  float front = (float)g->facing;
  float from[3][2] = {{g->x + front * g->w * 0.3f, g->y - g->h - 4.0f},
                      {g->x + front * (g->w * 0.5f + 6.0f), g->y - g->h * 0.6f},
                      {g->x, g->y - g->h * 0.5f}};
  /* He is no marksman: he aims somewhere within BANANA_MISS of Fabio, so
   * Fabio can often step out of the way, or be lucky standing still. */
  const player_t* p = &world->player;
  float tx = p->x + (rng_float(&world->rng) * 2.0f - 1.0f) * BANANA_MISS;
  float ty = p->y - PLAYER_H * 0.5f;
  int pick = 0;
  while (pick < 2 && !clear_start(world, from[pick][0], from[pick][1], tx, ty)) pick++;
  float x = from[pick][0], y = from[pick][1], vx, vy;
  aim_banana(x, y, tx, ty, &vx, &vy);
  bool slop = g->kind == ENEMY_SLOP;
  *banana = (banana_t){x, y, vx, vy, 0.0f, (int)pool_index_of(&world->enemies, g), slop};
  world_emit(world, slop ? EVENT_SLOP_THROWN : EVENT_BANANA_THROWN, x);
}

void world_cast_fireball(world_t* world, const enemy_t* d) {
  fireball_t* ball = pool_acquire(&world->fireballs, NULL);
  if (!ball) return;
  const player_t* p = &world->player;
  float x = d->x + (float)d->facing * 38.0f, y = d->y - 36.0f; /* the spark at his fist */
  if (level_solid_at(&world->level, x, y)) x = d->x; /* from his chest, against a wall */
  float dx = p->x - x, dy = (p->y - PLAYER_H * 0.5f) - y;
  float length = fmaxf(1.0f, sqrtf(dx * dx + dy * dy));
  *ball = (fireball_t){x, y, dx / length * FIREBALL_SPEED, dy / length * FIREBALL_SPEED, 0.0f};
  world_emit(world, EVENT_DEMON_CASTS, x);
}

static void update_fireballs(world_t* world, float dt) {
  size_t cursor = 0;
  for (fireball_t* f; (f = pool_next_active(&world->fireballs, &cursor));) {
    f->age += dt;
    f->x += f->vx * dt;
    f->y += f->vy * dt;
    bool spent = f->age > 4.0f || level_solid_at(&world->level, f->x, f->y);
    frect_t box = {f->x - 7.0f, f->y - 7.0f, 14.0f, 14.0f};
    player_t* p = &world->player;
    if (!spent && p->alive && frect_overlaps(box, player_rect(p))) {
      player_hurt(world, f->x);
      spent = true;
    }
    if (spent) {
      world_burst(world, f->x, f->y, 8, 255, 150, 40, 160.0f);
      pool_release_object(&world->fireballs, f);
    }
  }
}

static void splat_banana(world_t* world, banana_t* banana) {
  if (banana->slop) {
    world_add_effect(world, FX_SLOP_SPLAT, banana->x, banana->y + 10.0f, 0.6f, 1);
    world_burst(world, banana->x, banana->y, 8, 210, 70, 230, 160.0f);
  } else {
    world_burst(world, banana->x, banana->y, 8, 250, 220, 60, 160.0f);
  }
  pool_release_object(&world->bananas, banana);
}

/* The first one it touches dies, anybody but its gorilla, except Fabio, who
 * only loses a heart. Walls and the ground squash it. */
static void update_bananas(world_t* world, float dt) {
  size_t cursor = 0;
  for (banana_t* b; (b = pool_next_active(&world->bananas, &cursor));) {
    b->age += dt;
    b->vy += BANANA_GRAVITY * dt;
    b->x += b->vx * dt;
    b->y += b->vy * dt;
    if (b->age > BANANA_LIFE || b->y > level_height() + 60.0f) {
      pool_release_object(&world->bananas, b);
      continue;
    }
    if (level_solid_at(&world->level, b->x, b->y)) {
      splat_banana(world, b);
      continue;
    }
    frect_t box = {b->x - BANANA_SIZE * 0.5f, b->y - BANANA_SIZE * 0.5f, BANANA_SIZE,
                   BANANA_SIZE};
    player_t* p = &world->player;
    if (p->alive && frect_overlaps(box, player_rect(p))) {
      player_hurt(world, b->x);
      splat_banana(world, b);
      continue;
    }
    if (b->slop) continue; /* slop hurts Fabio and nobody else */
    size_t target_cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &target_cursor));) {
      if ((int)pool_index_of(&world->enemies, e) == b->thrower) continue;
      if (frect_overlaps(box, enemy_rect(e))) {
        world_kill_enemy(world, e, false);
        splat_banana(world, b);
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

/* The chef's blade reaches out in front of him, past his own body. */
static void chef_knives(world_t* world) {
  player_t* p = &world->player;
  if (!p->alive) return;
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (e->kind != ENEMY_CHEF || !chef_knife_out(e) || enemy_in_bubble(world, e)) continue;
    float reach = (float)e->facing * CHEF_KNIFE_REACH;
    frect_t blade = {fminf(e->x, e->x + reach), e->y - 46.0f, fabsf(reach), 34.0f};
    if (frect_overlaps(inset(player_rect(p), 3.0f), blade)) player_hurt(world, e->x);
  }
}

static void touch_enemies(world_t* world) {
  player_t* p = &world->player;
  if (!p->alive) return;
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    if (e->kind == ENEMY_BOOK_READER || enemy_in_bubble(world, e)) continue;
    if (e->kind == ENEMY_DEMON && demon_presence(e) < 1.0f) continue; /* not all there yet */
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
  update_skewers(world, dt);
  update_bananas(world, dt);
  update_fireballs(world, dt);
  update_debris(world, dt);
  update_melee(world);
  touch_enemies(world);
  chef_knives(world);

  size_t cursor = 0;
  for (ray_t* ray; (ray = pool_next_active(&world->rays, &cursor));) {
    ray->age += dt;
    if (ray->age > 0.22f) pool_release_object(&world->rays, ray);
  }
  cursor = 0;
  for (effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));) {
    float before = fx->age;
    fx->age += dt;
    /* Out of the hooded monk's smoke rises a demon, where he stood. */
    if (fx->kind == FX_MONK_BOOM && before < DEMON_RISE_DELAY && fx->age >= DEMON_RISE_DELAY)
      spawn_demon(world, fx->x, fx->y);
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
