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
#define CATCH_UP_FROM 260.0f   /* further than this from where he should be, he sprints to catch up */
#define CATCH_UP_SPEED (RUN_SPEED * 2.2f)
#define LOST_FOR_GOOD 1500.0f  /* off the screen and this far from Fabio: he runs back in from its edge */
#define TRAVEL_SPEED 40.0f     /* Fabio is travelling (not just turning) above this speed */
#define FACE_MOTION 60.0f      /* a friend faces the way he runs above this speed, and the way Fabio goes below it */
#define FLOOR_GAP 24.0f        /* higher or lower than this and he is on another floor than Fabio */
#define PLAN_EVERY 0.25f       /* how often he works out his way again */
#define PLANS_PER_UPDATE 1     /* friends who may work out a route in one update */
#define JUMP_AT 14.0f          /* how near the take-off spot he jumps */

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

/* Giacomo, if he is here, on his feet and past his moment of grace. Giulia punches him every time
 * she reaches him. */
static ally_t* giulia_target(world_t* world, const ally_t* giulia) {
  if (giulia->kind != ALLY_GIULIA || giulia->age < ALLY_ARRIVE_SECONDS) return NULL;
  ally_t* g = friend_of_kind(world, ALLY_GIACOMO);
  return g && !g->sitting && g->grace <= 0.0f && g->age >= ALLY_ARRIVE_SECONDS ? g : NULL;
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

/* A friend up on a block or a ledge, with Fabio on it too, must not stroll off the edge after his
 * place in the formation (a long climb back): the target stops at the brink of a drop of more than
 * two tiles onto a lower floor. Stairs, a hole (he leaps those) and a wall (he hops those) are no
 * such drop. */
static float keep_to_the_floor(const world_t* world, const ally_t* a, float target) {
  int row0 = (int)floorf(a->y / TILE);
  int col = (int)floorf(a->x / TILE), last = (int)floorf(target / TILE);
  if (last == col) return target;
  int dir = last > col ? 1 : -1;
  for (int c = col + dir; c != last + dir; c += dir) {
    int row = row0;
    while (row < LEVEL_ROWS && !tile_is_solid(level_tile(&world->level, c, row))) row++;
    if (row >= LEVEL_ROWS) continue; /* a hole */
    if ((float)((row - row0) * TILE) > (float)(2 * TILE)) return dir > 0 ? (float)(c * TILE) - ALLY_W : (float)((c + 1) * TILE) + ALLY_W;
  }
  return target;
}

/* The way Fabio is travelling: where he is really heading, not just which way he faces (he may shoot
 * or strike backwards while walking on). When he is standing still it stays as it was. */
static int update_lead(world_t* world) {
  const player_t* p = &world->player;
  if (fabsf(p->vx) > TRAVEL_SPEED) world->ally_lead = p->vx > 0.0f ? 1 : -1;
  else if (world->ally_lead == 0) world->ally_lead = p->facing;
  return world->ally_lead;
}

static bool in_view(const world_t* world, float x) {
  float view = world->view_w > 0.0f ? world->view_w : 1280.0f;
  return x > world->camera_x - 40.0f && x < world->camera_x + view + 40.0f;
}

/* He arrives right at Fabio, a step behind him, and runs on ahead to his place from there. */
static void appear_at_fabio(const world_t* world, ally_t* a) {
  const player_t* p = &world->player;
  a->x = p->x - (float)world->ally_lead * 14.0f;
  a->y = p->y;
  a->vx = a->vy = 0.0f;
  a->platform = -1;
  a->planned = false;
  a->plan_in = 0.0f;
}

/* The top of the ground in a column (the lowest floor there is), if it has any, with room above it:
 * not the rock outside a cave, not a hole. */
static bool ground_top(const world_t* world, float x, float* top) {
  int col = (int)floorf(x / TILE);
  if (col < 0 || col >= world->level.cols || col >= LEVEL_COLS) return false;
  if (!tile_is_solid(level_tile(&world->level, col, LEVEL_ROWS - 1))) return false;
  int row = LEVEL_ROWS - 1;
  while (row >= 0 && tile_is_solid(level_tile(&world->level, col, row))) row--;
  if (row < 2) return false;
  *top = (float)((row + 1) * TILE);
  return true;
}

/* Out beyond one edge of the screen, the first column with ground. */
static bool entry_beyond(const world_t* world, float edge, float outward, float* x, float* top) {
  for (int i = 0; i < 24; i++) {
    float at = edge + outward * (float)(i * TILE);
    if (ground_top(world, at, top)) {
      *x = at;
      return true;
    }
  }
  return false;
}

/* A friend who fell down a hole, or is far off and out of sight, comes back the way he would if he
 * could: running in from just beyond the edge of the screen, behind Fabio (or, if the level has no
 * ground there, ahead of him), on the ground, to catch him up. Where nobody can see him move. If the
 * screen shows the level end to end there is nowhere to come in from, and he does not: see allies_update. */
static bool run_back_in(const world_t* world, ally_t* a) {
  float view = world->view_w > 0.0f ? world->view_w : 1280.0f;
  float left_edge = world->camera_x - 90.0f, right_edge = world->camera_x + view + 90.0f;
  float behind = -(float)world->ally_lead; /* -1 or 1: which side of the screen is behind Fabio */
  float x = 0.0f, top = 0.0f;
  bool found = false;
  for (int side = 0; side < 2 && !found; side++) {
    float outward = side == 0 ? behind : -behind;
    found = entry_beyond(world, outward < 0.0f ? left_edge : right_edge, outward, &x, &top);
    if (found) a->vx = -outward * RUN_SPEED;
  }
  if (!found) return false;
  a->x = x;
  a->y = top;
  a->vy = 0.0f;
  a->platform = -1;
  a->planned = false;
  a->plan_in = 0.0f;
  return true;
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
  if (pool_get_active_count(&world->allies) == 0 || world->ally_lead == 0) world->ally_lead = world->player.facing;
  *a = (ally_t){.kind = kind, .facing = world->ally_lead, .left = ALLY_SECONDS, .platform = -1};
  appear_at_fabio(world, a);
  world_add_effect(world, FX_ALLY_POOF, a->x, a->y, 0.5f, 1);
  world_burst(world, a->x, a->y - 28.0f, 10, 255, 240, 160, 200.0f);
  world_emit(world, EVENT_ALLY_ARRIVES, a->x);
}

void allies_regroup(world_t* world) {
  size_t cursor = 0;
  for (ally_t* a; (a = pool_next_active(&world->allies, &cursor));) appear_at_fabio(world, a);
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
  int lead = update_lead(world);
  int plans_this_update = 0;
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
    a->grace = fmaxf(0.0f, a->grace - dt);
    if (a->sitting && a->grounded) { /* sits it out, then gets up where he sat */
      a->sit_left -= dt;
      if (a->sit_left <= 0.0f) {
        a->sitting = false;
        a->grace = GIACOMO_GRACE_SECONDS; /* a moment to get his breath back, and his bearings */
        a->cooldown = 0.4f;
        world_burst(world, a->x, a->y - 20.0f, 6, 255, 240, 160, 120.0f);
      }
    }
    bool here = a->age >= ALLY_ARRIVE_SECONDS; /* he is still taking shape as he turns up */

    /* Where he wants to be: his place ahead of Fabio, or in reach of the nearest enemy. */
    float target = p->x + (float)lead * kAlly[a->kind].offset;
    ally_t* victim = giulia_target(world, a); /* Giulia has a score to settle with Giacomo */
    const enemy_t* prey = here && !victim && !a->sitting ? nearest_enemy(world, a, kAlly[a->kind].chase > 0.0f ? kAlly[a->kind].chase : kAlly[a->kind].reach) : NULL;
    int facing = lead;
    if (prey) {
      facing = prey->x < a->x ? -1 : 1;
      if (kAlly[a->kind].chase > 0.0f) target = prey->x - (float)facing * kAlly[a->kind].reach * 0.55f;
    }
    if (victim) {
      facing = victim->x < a->x ? -1 : 1;
      target = victim->x - (float)facing * kAlly[a->kind].reach * 0.55f;
    }
    if (a->sitting) target = a->x; /* punched: he stays where he sits */
    bool held_by_the_edge = false; /* the floor under him ends before his place does */
    /* On another floor than Fabio he finds his own way there, one jump at a time. */
    bool same_support = p->platform == a->platform; /* the same plank, or both on tiles */
    bool other_floor = p->grounded && (!same_support || (p->platform < 0 && fabsf(p->y - a->y) > FLOOR_GAP));
    if (!other_floor && a->grounded) { /* standing where Fabio does: nothing to plan */
      a->planned = false;
      a->plan_in = 0.0f;
    } else if (other_floor && here && !a->sitting && a->grounded && a->attack <= 0.0f) {
      a->plan_in -= dt;
      if (a->plan_in <= 0.0f && plans_this_update < PLANS_PER_UPDATE) {
        plans_this_update++; /* the search is not free: one friend per update, the others the next */
        ally_step_t step = ally_plan_step(world, a);
        a->planned = step.found;
        a->go_x = step.go_x;
        a->land_x = step.land_x;
        a->jump_next = step.jump;
        a->plan_in = PLAN_EVERY;
      }
    }
    if (a->planned && !a->sitting && (other_floor || !a->grounded)) {
      if (a->grounded) {
        target = a->jump_next ? a->go_x : a->land_x; /* to the take-off spot, or on over the edge */
        if (a->jump_next && fabsf(a->go_x - a->x) < JUMP_AT && a->attack <= 0.0f) {
          a->vy = -JUMP_SPEED;
          a->vx = (a->land_x >= a->x ? 1.0f : -1.0f) * RUN_SPEED;
          a->grounded = false;
        }
      } else {
        target = a->land_x;
        a->plan_in = 0.0f; /* the moment he lands he works out the next move afresh, not from the old plan */
      }
    } else if (a->platform >= 0 && a->platform == p->platform) { /* up there with him, he keeps to the plank */
      const platform_def_t* def = &world->level.platforms[a->platform];
      const plank_t* plank = &world->planks[a->platform];
      float half = def->w * 0.5f - 12.0f;
      float wanted = target;
      target = clampf(target, plank->x - half, plank->x + half);
      held_by_the_edge = target != wanted;
    } else if (a->grounded && a->platform < 0) {
      float wanted = target;
      target = keep_to_the_floor(world, a, target);
      held_by_the_edge = target != wanted;
    }
    if (a->attack > 0.0f) target = a->x; /* he stands still while he strikes */
    float dx = target - a->x;
    float pace = fabsf(dx) > CATCH_UP_FROM ? CATCH_UP_SPEED : RUN_SPEED * 1.2f; /* far behind: he sprints */
    /* Keeping his place ahead of Fabio he moves at Fabio's own speed plus whatever closes the gap: left
     * to the gap alone he would trail his place by a seventh of a second of Fabio's walk. */
    bool keeping_place = !prey && !victim && !(a->planned && !a->sitting && (other_floor || !a->grounded)) && a->attack <= 0.0f && !a->sitting;
    float with_fabio = keeping_place && !held_by_the_edge ? p->vx : 0.0f; /* held at an edge: no running on past it */
    float want = clampf(dx * 5.0f + with_fabio, -pace, pace);
    if (fabsf(dx) < 6.0f && fabsf(with_fabio) < 20.0f) want = 0.0f;
    a->vx = approachf(a->vx, want, 3200.0f * dt);
    if (a->attack <= 0.0f && !a->sitting) a->facing = fabsf(a->vx) > FACE_MOTION && !prey && !victim ? (a->vx > 0.0f ? 1 : -1) : facing;
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
    bool below_the_screen = a->y - ALLY_H > level_height() + 8.0f; /* all of him is below the bottom edge */
    bool gone = !in_view(world, a->x) && fabsf(a->x - p->x) > LOST_FOR_GOOD;
    if ((below_the_screen || gone) && !run_back_in(world, a) && below_the_screen) {
      /* Nowhere to come in from, and nobody is to see him move: he waits below the screen, under Fabio, until
       * there is (the screen shows less of the level). */
      a->x = p->x;
      a->y = level_height() + ALLY_H + 30.0f;
      a->vx = a->vy = 0.0f;
      a->planned = false;
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
