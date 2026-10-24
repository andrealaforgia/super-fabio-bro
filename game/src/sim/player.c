#include <math.h>

#include "world.h"

#define COYOTE_TIME 0.09f
#define JUMP_BUFFER_TIME 0.11f
#define CUT_JUMP_SPEED 300.0f
#define INVULNERABLE_AFTER_HIT 1.7f
#define GUN_COOLDOWN 0.28f
#define SPELL_COOLDOWN 0.34f
#define MELEE_TIME 0.34f
#define MELEE_COOLDOWN 0.15f
#define DRINK_TIME 0.65f
#define MAX_POTIONS 5

rect_t player_rect(const player_t* p) {
  rect_t r = {p->x - PLAYER_W * 0.5f, p->y - PLAYER_H, PLAYER_W, PLAYER_H};
  return r;
}

void player_reset(player_t* p, const level_t* level) {
  *p = (player_t){0};
  p->x = level->spawn_x;
  p->y = level->spawn_y;
  p->facing = 1;
  p->alive = true;
  p->hp = PLAYER_HP;
  p->safe_x = p->x;
  p->safe_y = p->y;
}

static void die(world_t* world) {
  player_t* p = &world->player;
  p->alive = false;
  p->hp = 0;
  p->vy = -560.0f;
  p->vx = 0.0f;
  p->dead_time = 0.0f;
  world->state = STATE_DYING;
  world->state_time = 0.0f;
  world_emit(world, EVENT_DEATH, p->x);
}

void player_hurt(world_t* world, float from_x) {
  player_t* p = &world->player;
  if (!p->alive || p->invulnerable > 0.0f) return;
  p->hp--;
  p->invulnerable = INVULNERABLE_AFTER_HIT;
  p->hurt_timer = 0.4f;
  p->melee_timer = 0.0f;
  world_emit(world, EVENT_HURT, p->x);
  if (p->hp <= 0) {
    die(world);
    return;
  }
  float away = p->x < from_x ? -1.0f : 1.0f;
  p->vx = away * 280.0f;
  p->vy = -330.0f;
  p->grounded = false;
}

static void remember_safe_ground(const level_t* level, player_t* p) {
  if (!p->grounded) return;
  int row = (int)(p->y / TILE);
  int a = (int)floorf((p->x - PLAYER_W * 0.5f) / TILE);
  int b = (int)floorf((p->x + PLAYER_W * 0.5f) / TILE);
  if (tile_is_solid(level_tile(level, a, row)) &&
      tile_is_solid(level_tile(level, b, row))) {
    p->safe_x = p->x;
    p->safe_y = p->y;
  }
}

static void fall_into_pit(world_t* world) {
  player_t* p = &world->player;
  p->hp--;
  world_emit(world, EVENT_HURT, p->x);
  if (p->hp <= 0) {
    die(world);
    return;
  }
  p->x = p->safe_x;
  p->y = p->safe_y;
  p->vx = p->vy = 0.0f;
  p->invulnerable = 2.0f;
}

static void drink(world_t* world) {
  player_t* p = &world->player;
  p->potions--;
  p->spell_time = POTION_SECONDS;
  p->drink_timer = DRINK_TIME;
  world_emit(world, EVENT_DRINK, p->x);
}

static void actions(world_t* world, const controls_t* controls, float dt) {
  player_t* p = &world->player;
  (void)dt;
  if (controls->pressed.drink && p->potions > 0 && p->spell_time <= 0.0f &&
      p->drink_timer <= 0.0f)
    drink(world);

  bool spell = p->spell_time > 0.0f;
  if (controls->held.shoot && p->gun_cooldown <= 0.0f && p->drink_timer <= 0.0f) {
    if (spell) {
      world_cast_spell(world);
      p->gun_cooldown = SPELL_COOLDOWN;
    } else if (p->has_gun) {
      world_fire_bullet(world);
      p->gun_cooldown = GUN_COOLDOWN;
    }
  }

  if (controls->pressed.melee && p->melee_timer <= 0.0f &&
      p->melee_cooldown <= 0.0f) {
    p->melee_timer = MELEE_TIME;
    p->melee_struck = false;
    world_emit(world, EVENT_MELEE, p->x);
  }
}

void player_update(world_t* world, const controls_t* controls, float dt) {
  player_t* p = &world->player;
  const level_t* level = &world->level;
  p->anim_time += dt;

  if (!p->alive) {
    p->dead_time += dt;
    p->vy = fminf(p->vy + GRAVITY * dt, MAX_FALL);
    p->y += p->vy * dt;
    return;
  }

  p->invulnerable = fmaxf(0.0f, p->invulnerable - dt);
  p->hurt_timer = fmaxf(0.0f, p->hurt_timer - dt);
  p->gun_cooldown = fmaxf(0.0f, p->gun_cooldown - dt);
  p->melee_cooldown = fmaxf(0.0f, p->melee_cooldown - dt);
  p->shoot_pose = fmaxf(0.0f, p->shoot_pose - dt);
  p->cast_pose = fmaxf(0.0f, p->cast_pose - dt);
  p->drink_timer = fmaxf(0.0f, p->drink_timer - dt);
  if (p->melee_timer > 0.0f) {
    p->melee_timer = fmaxf(0.0f, p->melee_timer - dt);
    if (p->melee_timer <= 0.0f) p->melee_cooldown = MELEE_COOLDOWN;
  }
  if (p->spell_time > 0.0f) p->spell_time = fmaxf(0.0f, p->spell_time - dt);

  float direction = (controls->held.right ? 1.0f : 0.0f) -
                    (controls->held.left ? 1.0f : 0.0f);
  float top_speed = controls->held.run ? RUN_SPEED : WALK_SPEED;
  if (p->hurt_timer <= 0.0f) {
    if (direction != 0.0f) {
      float accel = p->grounded ? 2400.0f : 1500.0f;
      /* Turning round on the spot is quicker than speeding up. */
      if ((direction > 0.0f) != (p->vx > 0.0f) && fabsf(p->vx) > 10.0f)
        accel *= 1.8f;
      p->vx = approachf(p->vx, direction * top_speed, accel * dt);
      if (p->melee_timer <= 0.0f) p->facing = direction > 0.0f ? 1 : -1;
    } else {
      p->vx = approachf(p->vx, 0.0f, (p->grounded ? 2800.0f : 500.0f) * dt);
    }
  }

  p->coyote = p->grounded ? COYOTE_TIME : p->coyote - dt;
  p->jump_buffer = controls->pressed.jump ? JUMP_BUFFER_TIME : p->jump_buffer - dt;
  if (p->jump_buffer > 0.0f && p->coyote > 0.0f && p->hurt_timer <= 0.0f) {
    p->vy = -JUMP_SPEED;
    p->coyote = 0.0f;
    p->jump_buffer = 0.0f;
    p->grounded = false;
    world_emit(world, EVENT_JUMP, p->x);
  }
  p->vy = fminf(p->vy + GRAVITY * dt, MAX_FALL);
  /* Letting go of jump early makes a short hop. */
  if (!controls->held.jump && p->vy < -CUT_JUMP_SPEED && p->hurt_timer <= 0.0f)
    p->vy += GRAVITY * 2.0f * dt;

  actions(world, controls, dt);

  move_result_t moved = physics_move(level, &p->x, &p->y, &p->vx, &p->vy,
                                     PLAYER_W, PLAYER_H, dt);
  p->grounded = moved.grounded;
  if (moved.hit_ceiling) world_bump_block(world, moved.ceiling_col, moved.ceiling_row);

  if (p->grounded && fabsf(p->vx) > 8.0f)
    p->walk_phase += fabsf(p->vx) * dt * 0.075f;
  remember_safe_ground(level, p);

  if (p->y > level_height() + 96.0f) fall_into_pit(world);
  if (p->x < PLAYER_W * 0.5f) p->x = PLAYER_W * 0.5f;
  if (p->x > level_width() - PLAYER_W * 0.5f)
    p->x = level_width() - PLAYER_W * 0.5f;
}
