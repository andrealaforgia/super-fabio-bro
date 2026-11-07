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

frect_t player_rect(const player_t* p) {
  frect_t r = {p->x - PLAYER_W * 0.5f, p->y - PLAYER_H, PLAYER_W, PLAYER_H};
  return r;
}

void player_reset(player_t* p, const level_t* level) {
  *p = (player_t){0};
  p->x = level->spawn_x;
  p->y = level->spawn_y;
  p->facing = 1;
  p->platform = -1;
  p->alive = true;
  p->hp = PLAYER_HP;
  p->safe_x = p->x;
  p->safe_y = p->y;
}

static void die(world_t* world) {
  player_t* p = &world->player;
  p->alive = false;
  p->flying = false;
  p->chute = false;
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
  if (!p->alive || p->invulnerable > 0.0f || p->flying) return; /* nothing touches him up there */
  if (world_drumming(world)) return; /* or while he is playing the drums */
  if (world->warp.phase != WARP_NONE) return; /* or going down a pipe */
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
  p->chute = false;
  p->platform = -1;
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
  if (controls_pressed(controls, BTN_DRINK) && p->potions > 0 && p->spell_time <= 0.0f &&
      p->drink_timer <= 0.0f)
    drink(world);

  /* One shot at a time, and the weapons have a pecking order: the lightsaber's burst, the TDD spell's
   * blast first, then the skewers while he is in the helicopter, then the bubble shotgun, then the pistol. */
  bool spell = p->spell_time > 0.0f;
  if (controls_held(controls, BTN_SHOOT) && p->gun_cooldown <= 0.0f && p->drink_timer <= 0.0f) {
    if (p->jedi_time > 0.0f && !p->flying) {
      world_fire_force_burst(world);
      p->gun_cooldown = JEDI_COOLDOWN;
    } else if (spell) {
      world_cast_spell(world);
      p->gun_cooldown = SPELL_COOLDOWN;
    } else if (p->flying) {
      world_fire_skewer(world);
      p->gun_cooldown = SKEWER_COOLDOWN;
    } else if (p->bubbles > 0) {
      p->bubbles--;
      world_fire_bubbles(world);
      p->gun_cooldown = BUBBLE_COOLDOWN;
    } else if (p->ammo > 0) {
      p->ammo--;
      world_fire_bullet(world);
      p->gun_cooldown = GUN_COOLDOWN;
    }
  }

  if (controls_pressed(controls, BTN_MELEE) && p->melee_timer <= 0.0f &&
      p->melee_cooldown <= 0.0f) {
    p->melee_timer = MELEE_TIME;
    p->melee_struck = false;
    world_emit(world, EVENT_MELEE, p->x);
  }
}

static void jump_and_fall(world_t* world, const controls_t* controls, float dt) {
  player_t* p = &world->player;
  p->coyote = p->grounded ? COYOTE_TIME : p->coyote - dt;
  p->jump_buffer = controls_pressed(controls, BTN_JUMP) ? JUMP_BUFFER_TIME : p->jump_buffer - dt;
  if (p->jump_buffer > 0.0f && p->coyote > 0.0f && p->hurt_timer <= 0.0f) {
    p->vy = -JUMP_SPEED;
    p->coyote = 0.0f;
    p->jump_buffer = 0.0f;
    p->grounded = false;
    world_emit(world, EVENT_JUMP, p->x);
  }
  p->vy = fminf(p->vy + GRAVITY * dt, p->chute ? PARACHUTE_FALL : MAX_FALL);
  /* Letting go of jump early makes a short hop. */
  if (!controls_held(controls, BTN_JUMP) && p->vy < -CUT_JUMP_SPEED && p->hurt_timer <= 0.0f)
    p->vy += GRAVITY * 2.0f * dt;
}

/* In the helicopter there is no gravity: jump or up climbs, down sinks, and
 * with neither he hangs in the air. */
static void fly(player_t* p, const controls_t* controls, float dt) {
  float climb = (controls_held(controls, BTN_JUMP) || controls_held(controls, BTN_UP)) ? 1.0f : 0.0f;
  float sink = controls_held(controls, BTN_DOWN) ? 1.0f : 0.0f;
  p->vy = approachf(p->vy, (sink - climb) * HELICOPTER_SPEED, 1800.0f * dt);
  p->coyote = 0.0f;
  p->jump_buffer = 0.0f;
}

/* He stays on the screen, above the ground line (so never down into a pit). */
static void keep_flying_in_bounds(player_t* p) {
  float ground = (float)(GROUND_ROW * TILE);
  if (p->y < HELICOPTER_CEILING) {
    p->y = HELICOPTER_CEILING;
    if (p->vy < 0.0f) p->vy = 0.0f;
  }
  if (p->y > ground) {
    p->y = ground;
    if (p->vy > 0.0f) p->vy = 0.0f;
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

  if (world->warp.phase != WARP_NONE) { /* sinking into a pipe, or rising out of one */
    p->vx = p->vy = 0.0f;
    return;
  }
  if (world_drumming(world)) { /* sitting at the kit: drums_update keeps him there */
    p->vx = p->vy = 0.0f;
    return;
  }

  platforms_carry(world, p);
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
  if (p->jedi_time > 0.0f) {
    p->jedi_time = fmaxf(0.0f, p->jedi_time - dt);
    if (p->jedi_time <= 0.0f) world_burst(world, p->x, p->y - 30.0f, 12, 90, 255, 120, 220.0f); /* the robes fade */
  }

  float direction = (controls_held(controls, BTN_RIGHT) ? 1.0f : 0.0f) -
                    (controls_held(controls, BTN_LEFT) ? 1.0f : 0.0f);
  float top_speed = controls_held(controls, BTN_RUN) ? RUN_SPEED : WALK_SPEED;
  if (p->flying) top_speed *= HELICOPTER_SPEEDUP;
  if (p->chute) top_speed = WALK_SPEED * PARACHUTE_DRIFT;
  if (p->hurt_timer <= 0.0f) {
    if (direction != 0.0f) {
      float accel = p->grounded ? 2400.0f : 1500.0f;
      /* The snow zone is icy: he slides, slow to get going and slow to stop. */
      if (p->grounded && level_biome_at(&world->level, (int)(p->x / TILE)) == BIOME_SNOW) accel *= 0.3f;
      /* Turning round on the spot is quicker than speeding up. */
      if ((direction > 0.0f) != (p->vx > 0.0f) && fabsf(p->vx) > 10.0f)
        accel *= 1.8f;
      p->vx = approachf(p->vx, direction * top_speed, accel * dt);
      if (p->melee_timer <= 0.0f) p->facing = direction > 0.0f ? 1 : -1;
    } else {
      float brake = p->grounded ? 2800.0f : 500.0f;
      if (p->grounded && level_biome_at(&world->level, (int)(p->x / TILE)) == BIOME_SNOW) brake *= 0.15f;
      p->vx = approachf(p->vx, 0.0f, brake * dt);
    }
  }

  /* Jump out of the helicopter: tap jump (or up) twice quickly, or tap it holding down. */
  p->tap_window += dt;
  if (p->flying && (controls_pressed(controls, BTN_JUMP) || controls_pressed(controls, BTN_UP))) {
    if (p->tap_window < 0.35f || controls_held(controls, BTN_DOWN)) helicopter_bail_out(world);
    else p->tap_window = 0.0f;
  }
  if (p->chute) p->chute_age += dt;
  p->landing = fmaxf(0.0f, p->landing - dt);
  if (p->flying) fly(p, controls, dt);
  else jump_and_fall(world, controls, dt);

  actions(world, controls, dt);

  float feet_before = p->y;
  move_result_t moved = physics_move(level, &p->x, &p->y, &p->vx, &p->vy,
                                     PLAYER_W, PLAYER_H, dt);
  p->grounded = moved.grounded;
  if (p->flying) keep_flying_in_bounds(p);
  platforms_land(world, p, feet_before);
  if (p->chute && (p->grounded || p->y > (float)(GROUND_ROW * TILE) + 8.0f)) { /* touchdown (or into a pit) */
    p->chute = false;
    if (p->grounded) {
      p->landing = PARACHUTE_LANDING_SECONDS;
      p->vx = 0.0f;
      world_add_effect(world, FX_CHUTE_REMAINS, p->x - (float)p->facing * 30.0f, p->y, PARACHUTE_REMAINS_SECONDS, p->facing);
    }
  }
  if (moved.hit_ceiling) world_bump_block(world, moved.ceiling_col, moved.ceiling_row);

  if (p->grounded && fabsf(p->vx) > 8.0f)
    p->walk_phase += fabsf(p->vx) * dt * 0.075f;
  remember_safe_ground(level, p);

  if (p->y > level_height() + 96.0f) fall_into_pit(world);
  if (p->x < PLAYER_W * 0.5f) p->x = PLAYER_W * 0.5f;
  if (p->x > level_width() - PLAYER_W * 0.5f)
    p->x = level_width() - PLAYER_W * 0.5f;
}
