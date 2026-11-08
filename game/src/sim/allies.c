#include <math.h>

#include "world.h"

/* What sets the three friends apart. */
static const struct {
  float offset;   /* how far ahead of Fabio he stands */
  float chase;    /* how far off an enemy gets him running at it (0: he never leaves his place) */
  float reach;    /* how close an enemy has to be to be struck */
  float cooldown; /* between strikes */
  float attack;   /* how long a strike takes */
} kAlly[ALLY_KIND_COUNT] = {
    {90.0f, 0.0f, 95.0f, 1.1f, 0.9f},    /* Andrea: a wobble all round him */
    {150.0f, 260.0f, 90.0f, 0.55f, 0.5f}, /* Alessandro: kung fu */
    {210.0f, 230.0f, 78.0f, 0.42f, 0.45f}, /* Giulia: boxing */
    {270.0f, 0.0f, 560.0f, 0.9f, 0.6f}    /* Giacomo: stays put and throws the ball from afar */
};
#define STRIKE_AT 0.5f   /* how far into a strike it lands */
#define TELEPORT_AWAY 560.0f

ally_kind_t ally_for_item(item_kind_t kind) {
  return kind == ITEM_SUSHI ? ALLY_ANDREA : kind == ITEM_DRAGON ? ALLY_ALESSANDRO : kind == ITEM_GLOVES ? ALLY_GIULIA : ALLY_GIACOMO;
}

float ally_offset(ally_kind_t kind) { return kAlly[kind].offset; }
float ally_attack_seconds(ally_kind_t kind) { return kAlly[kind].attack; }

static bool circle_touches(float cx, float cy, float r, frect_t box) {
  float nx = clampf(cx, box.x, box.x + box.w), ny = clampf(cy, box.y, box.y + box.h);
  return (cx - nx) * (cx - nx) + (cy - ny) * (cy - ny) <= r * r;
}

/* The ground he strikes: Andrea's circle round his belly, or a box out in front (and a little
 * behind, for the spinning kick) for the other two. */
static bool strikes(const ally_t* a, const enemy_t* e) {
  frect_t box = enemy_rect(e);
  if (a->kind == ALLY_GIACOMO) return (box.x + box.w * 0.5f - a->x) * (float)a->facing > 0.0f && fabsf(box.x + box.w * 0.5f - a->x) <= kAlly[a->kind].reach;
  if (a->kind == ALLY_ANDREA) return circle_touches(a->x, a->y - 28.0f, kAlly[a->kind].reach, box);
  float reach = kAlly[a->kind].reach, behind = a->kind == ALLY_ALESSANDRO ? 40.0f : 10.0f;
  float left = a->facing > 0 ? a->x - behind : a->x - reach;
  frect_t hit = {left, a->y - 70.0f, reach + behind, 74.0f};
  return frect_overlaps(hit, box);
}

static ally_t* friend_of_kind(world_t* world, ally_kind_t kind) {
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));)
    if (a->kind == kind) return a;
  return NULL;
}

/* Giacomo, if he is here and on his feet. Giulia punches him every time she reaches him. */
static ally_t* giulia_target(world_t* world, const ally_t* giulia) {
  if (giulia->kind != ALLY_GIULIA || giulia->age < ALLY_ARRIVE_SECONDS) return NULL;
  ally_t* g = friend_of_kind(world, ALLY_GIACOMO);
  return g && !g->sitting && g->age >= ALLY_ARRIVE_SECONDS ? g : NULL;
}

/* Whether her glove is on him: `slack` is how much further he may have run in the half second she
 * takes to wind up and hit (Giacomo runs on while she punches). */
static bool giulia_reaches(const ally_t* giulia, const ally_t* g, float slack) {
  return fabsf(g->x - giulia->x) <= kAlly[ALLY_GIULIA].reach + slack && fabsf(g->y - giulia->y) < 40.0f;
}

/* A hole: a column with no ground in it from the feet to the bottom of the world. */
static bool is_pit_at(const world_t* world, float x, float feet) {
  int col = (int)floorf(x / TILE);
  for (int row = (int)floorf((feet - 1.0f) / TILE); row < LEVEL_ROWS; row++)
    if (tile_is_solid(level_tile(&world->level, col, row))) return false;
  return true;
}

static void place_beside_fabio(const world_t* world, ally_t* a) {
  const player_t* p = &world->player;
  a->x = p->x + (float)p->facing * kAlly[a->kind].offset;
  if (level_solid_at(&world->level, a->x, p->y - 10.0f)) a->x = p->x;
  a->y = p->y;
  a->vx = a->vy = 0.0f;
  a->platform = -1;
}

/* Fabio is up on a balcony, a roof or a capsule and his friend is not: he hops up beside him. */
static void join_on_plank(world_t* world, ally_t* a) {
  const player_t* p = &world->player;
  const platform_def_t* def = &world->level.platforms[p->platform];
  const plank_t* plank = &world->planks[p->platform];
  float half = def->w * 0.5f - 12.0f;
  a->x = clampf(p->x + (float)p->facing * fminf(kAlly[a->kind].offset * 0.3f, half), plank->x - half, plank->x + half);
  a->y = plank->top;
  a->vx = a->vy = 0.0f;
  a->grounded = true;
  a->platform = p->platform;
  world_burst(world, a->x, a->y - 28.0f, 6, 255, 240, 160, 160.0f);
}

void world_summon_ally(world_t* world, ally_kind_t kind) {
  ally_t* found = NULL;
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));)
    if (a->kind == kind) found = a;
  if (found) { /* he is here already: another ALLY_SECONDS */
    found->left = ALLY_SECONDS;
    return;
  }
  ally_t* a = pool_acquire(&world->allies, NULL);
  if (!a) return;
  *a = (ally_t){kind, 0, 0, 0, 0, world->player.facing, false, ALLY_SECONDS, 0.0f, 0.0f, 0.0f, 0.0f, false, -1, false, false, 0.0f};
  place_beside_fabio(world, a);
  world_add_effect(world, FX_ALLY_POOF, a->x, a->y, 0.5f, 1);
  world_burst(world, a->x, a->y - 28.0f, 10, 255, 240, 160, 200.0f);
  world_emit(world, EVENT_ALLY_ARRIVES, a->x);
}

void allies_regroup(world_t* world) {
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) place_beside_fabio(world, a);
}

/* The nearest enemy he has any business with, on about his own level. */
static const enemy_t* nearest_enemy(const world_t* world, const ally_t* a, float range) {
  const enemy_t* best = NULL;
  float best_d = range;
  size_t cursor = 0;
  for (const enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    float d = fabsf(e->x - a->x);
    if (d < best_d && fabsf((e->y - e->h * 0.5f) - (a->y - 28.0f)) < 110.0f) {
      best = e;
      best_d = d;
    }
  }
  return best;
}

/* Whether the enemy creeps along the floor, too low for a ball thrown standing up: the ground bugs. */
static bool crawls(const ally_t* a, const enemy_t* e) { return e->kind == ENEMY_BUG && e->h < 40.0f && e->y >= a->y - 8.0f; }

/* Giacomo lets the ball go. */
static void throw_the_ball(world_t* world, ally_t* a) {
  polo_ball_t* ball = pool_acquire(&world->polo_balls, NULL);
  float height = a->crouching ? 14.0f : 38.0f; /* crouched, he rolls it along the ground */
  if (ball) *ball = (polo_ball_t){a->x + (float)a->facing * 22.0f, a->y - height, (float)a->facing * POLO_BALL_SPEED, 0.0f};
  world_emit(world, EVENT_ALLY_THROW, a->x);
}

/* Giulia's glove finds Giacomo: he is thrown back and ends up sitting on the ground. */
static void punch_giacomo(world_t* world, ally_t* giulia, ally_t* g) {
  g->sitting = true;
  g->sit_left = GIACOMO_SIT_SECONDS;
  g->attack = 0.0f;
  g->vx = (float)giulia->facing * 320.0f;
  g->vy = -380.0f;
  g->grounded = false;
  g->platform = -1;
  g->facing = -giulia->facing;
  world_add_effect(world, FX_ALLY_PUNCH, g->x - (float)giulia->facing * 14.0f, g->y - 40.0f, 0.35f, giulia->facing);
  world_burst(world, g->x, g->y - 50.0f, 12, 255, 230, 90, 240.0f);
  world_emit(world, EVENT_ALLY_PUNCH, g->x);
  world_emit(world, EVENT_ALLY_OUCH, g->x);
}

void polo_balls_update(world_t* world, float dt) {
  size_t cursor = 0;
  for (polo_ball_t* b; (b = pool_next_active(&world->polo_balls, &cursor));) {
    b->age += dt;
    b->x += b->vx * dt;
    if (b->age > POLO_BALL_LIFE || level_solid_at(&world->level, b->x, b->y)) {
      world_burst(world, b->x, b->y, 6, 120, 200, 255, 160.0f);
      pool_release_object(&world->polo_balls, b);
      continue;
    }
    frect_t box = {b->x - POLO_BALL_R, b->y - POLO_BALL_R, POLO_BALL_R * 2.0f, POLO_BALL_R * 2.0f};
    size_t target_cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &target_cursor));) {
      if (!frect_overlaps(box, enemy_rect(e))) continue;
      world_burst(world, b->x, b->y, 10, 120, 200, 255, 220.0f); /* a splash */
      world_kill_enemy(world, e, true);
      pool_release_object(&world->polo_balls, b);
      break;
    }
  }
}

static void land_the_strike(world_t* world, ally_t* a) {
  a->struck = true;
  if (a->kind == ALLY_GIACOMO) {
    throw_the_ball(world, a);
    return;
  }
  if (a->kind == ALLY_GIULIA) {
    ally_t* g = giulia_target(world, a);
    if (g && giulia_reaches(a, g, 110.0f)) {
      punch_giacomo(world, a, g);
      return;
    }
  }
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
    if (strikes(a, e)) world_kill_enemy(world, e, false);
  float fx = a->x + (a->kind == ALLY_ANDREA ? 0.0f : (float)a->facing * kAlly[a->kind].reach * 0.6f);
  float fy = a->y - (a->kind == ALLY_ANDREA ? 30.0f : 34.0f);
  fx_kind_t kind = a->kind == ALLY_ANDREA ? FX_ALLY_WAVE : (a->kind == ALLY_ALESSANDRO ? FX_ALLY_SLASH : FX_ALLY_PUNCH);
  world_add_effect(world, kind, fx, fy, a->kind == ALLY_ANDREA ? 0.5f : 0.35f, a->facing); /* his lasts longer: it is the big one */
  if (a->kind == ALLY_ANDREA) world_burst(world, fx, fy, 16, 255, 230, 90, 280.0f); /* a spray of golden sparks */
  event_kind_t sound = a->kind == ALLY_ANDREA ? EVENT_ALLY_BELLY : (a->kind == ALLY_ALESSANDRO ? EVENT_ALLY_KUNGFU : EVENT_ALLY_PUNCH);
  world_emit(world, sound, a->x);
}

void allies_update(world_t* world, float dt) {
  const player_t* p = &world->player;
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) {
    a->left -= dt;
    a->age += dt;
    a->anim += dt;
    if (a->left <= 0.0f) {
      world_add_effect(world, FX_ALLY_POOF, a->x, a->y, 0.5f, 1);
      world_burst(world, a->x, a->y - 28.0f, 10, 255, 240, 160, 200.0f);
      pool_release_object(&world->allies, a);
      continue;
    }
    a->cooldown = fmaxf(0.0f, a->cooldown - dt);
    if (a->sitting && a->grounded) { /* sits it out, then gets up where he sat */
      a->sit_left -= dt;
      if (a->sit_left <= 0.0f) {
        a->sitting = false;
        a->cooldown = 0.4f;
        world_burst(world, a->x, a->y - 20.0f, 6, 255, 240, 160, 120.0f);
      }
    }
    bool here = a->age >= ALLY_ARRIVE_SECONDS; /* he is still taking shape as he turns up */

    /* Where he wants to be: his place ahead of Fabio, or in reach of the nearest enemy. */
    float target = p->x + (float)p->facing * kAlly[a->kind].offset;
    ally_t* victim = giulia_target(world, a); /* Giulia has a score to settle with Giacomo */
    const enemy_t* prey = here && !victim && !a->sitting ? nearest_enemy(world, a, kAlly[a->kind].chase > 0.0f ? kAlly[a->kind].chase : kAlly[a->kind].reach) : NULL;
    int facing = p->facing;
    if (prey) {
      facing = prey->x < a->x ? -1 : 1;
      if (kAlly[a->kind].chase > 0.0f) target = prey->x - (float)facing * kAlly[a->kind].reach * 0.55f;
    }
    if (victim) {
      facing = victim->x < a->x ? -1 : 1;
      target = victim->x - (float)facing * kAlly[a->kind].reach * 0.55f;
    }
    if (a->sitting) target = a->x; /* punched: he stays where he sits */
    if (here && p->platform >= 0 && a->platform != p->platform && p->grounded) {
      join_on_plank(world, a);
      target = a->x;
    } else if (a->platform >= 0 && a->platform == p->platform) { /* up there, he keeps to the plank */
      const platform_def_t* def = &world->level.platforms[a->platform];
      const plank_t* plank = &world->planks[a->platform];
      float half = def->w * 0.5f - 12.0f;
      target = clampf(target, plank->x - half, plank->x + half);
    }
    if (a->attack > 0.0f) target = a->x; /* he stands still while he strikes */
    float dx = target - a->x;
    float want = clampf(dx * 5.0f, -RUN_SPEED * 1.2f, RUN_SPEED * 1.2f);
    if (fabsf(dx) < 6.0f) want = 0.0f;
    a->vx = approachf(a->vx, want, 3200.0f * dt);
    if (a->attack <= 0.0f && !a->sitting) a->facing = fabsf(a->vx) > 20.0f && !prey ? (a->vx > 0.0f ? 1 : -1) : facing;
    a->vy = fminf(a->vy + GRAVITY * dt, MAX_FALL);
    /* A hole in the way and he is running at it: he takes off at the edge and clears it. */
    if (a->grounded && a->attack <= 0.0f && fabsf(want) > 40.0f) {
      float dir = want > 0.0f ? 1.0f : -1.0f;
      if (is_pit_at(world, a->x + dir * (ALLY_W * 0.5f + 8.0f), a->y)) {
        a->vy = -JUMP_SPEED;
        a->vx = dir * RUN_SPEED * 1.2f; /* fast enough for the widest hole */
        a->grounded = false;
      }
    }
    platforms_carry_body(world, &a->x, &a->y, &a->platform);
    float feet_before = a->y;
    move_result_t move = physics_move(&world->level, &a->x, &a->y, &a->vx, &a->vy, ALLY_W, ALLY_H, dt);
    a->grounded = move.grounded;
    platforms_land_body(world, &a->x, &a->y, &a->vy, &a->grounded, &a->platform, feet_before);
    if (a->grounded && (move.hit_left || move.hit_right) && fabsf(want) > 40.0f) a->vy = -JUMP_SPEED * 0.85f; /* up and over */

    /* Lost: fallen into a pit, or left far behind. He turns up at Fabio's side again. */
    if (a->y > level_height() + 40.0f || fabsf(a->x - p->x) > TELEPORT_AWAY) {
      place_beside_fabio(world, a);
      world_burst(world, a->x, a->y - 28.0f, 6, 255, 240, 160, 160.0f);
    }

    /* The strike: begins when something is in reach, lands halfway through. */
    if (a->attack > 0.0f) {
      a->attack += dt;
      if (!a->struck && a->attack >= kAlly[a->kind].attack * STRIKE_AT) land_the_strike(world, a);
      if (a->attack >= kAlly[a->kind].attack) a->attack = 0.0f;
    } else if (here && a->cooldown <= 0.0f && !a->sitting &&
               ((prey && strikes(a, prey)) || (victim && giulia_reaches(a, victim, 0.0f)))) {
      a->attack = 0.0001f;
      a->struck = false;
      a->crouching = a->kind == ALLY_GIACOMO && prey && crawls(a, prey);
      a->cooldown = kAlly[a->kind].cooldown + kAlly[a->kind].attack;
      a->facing = facing;
    }
  }
}
