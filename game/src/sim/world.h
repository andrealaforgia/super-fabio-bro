/**
 * @file world.h
 * @brief The whole game state and its rules, with no drawing and no SDL:
 * Fabio, zombies, bugs, bullets, spells, pickups and the level. Everything
 * is advanced by `world_update` with explicit time, so tests can play it.
 *
 * Positions of characters are the middle of the feet (x) and the soles (y);
 * y grows downwards.
 */
#ifndef GAME_SIM_WORLD_H_
#define GAME_SIM_WORLD_H_

#include <stdbool.h>
#include <stdint.h>

#include "controls.h"
#include "geometry.h"
#include "level.h"
#include "rng.h"

#define MAX_ENEMIES 48
#define MAX_PICKUPS 32
#define MAX_BULLETS 16
#define MAX_RAYS 4
#define MAX_EFFECTS 48
#define MAX_PARTICLES 384
#define MAX_BUMPS 8
#define MAX_EVENTS 64

/* Tuning, in pixels and seconds. */
#define GRAVITY 2000.0f
#define MAX_FALL 900.0f
#define WALK_SPEED 175.0f
#define RUN_SPEED 285.0f
#define JUMP_SPEED 790.0f
#define PLAYER_W 24.0f
#define PLAYER_H 52.0f
#define PLAYER_HP 5
#define POTION_SECONDS 14.0f
#define SPELL_LENGTH 620.0f
#define BULLET_SPEED 760.0f

typedef enum {
  STATE_PLAYING,
  STATE_DYING,
  STATE_GAME_OVER,
  STATE_WON
} game_state_t;

typedef enum {
  EVENT_JUMP,
  EVENT_SHOOT,
  EVENT_SPELL,
  EVENT_MELEE,
  EVENT_ZOMBIE_DIES,
  EVENT_BUG_DIES,
  EVENT_COIN,
  EVENT_PICKUP,
  EVENT_DRINK,
  EVENT_HURT,
  EVENT_BUMP,
  EVENT_STOMP,
  EVENT_BLOCK_OPENS,
  EVENT_GROAN,
  EVENT_DEATH,
  EVENT_WIN,
  EVENT_HEAL
} event_kind_t;

typedef struct {
  event_kind_t kind;
  float x;
} event_t;

typedef struct {
  float x, y, vx, vy;
  int facing; /* +1 right, -1 left */
  bool grounded;
  bool alive;
  int hp;
  float invulnerable;
  float hurt_timer;
  bool has_gun;
  int potions;
  float spell_time; /* seconds of TDD spell left */
  float gun_cooldown;
  float melee_timer;
  float melee_cooldown;
  bool melee_struck;
  float drink_timer;
  float shoot_pose;
  float cast_pose;
  float coyote;
  float jump_buffer;
  float walk_phase;
  float anim_time;
  float safe_x, safe_y;
  float dead_time;
} player_t;

typedef enum { ENEMY_ZOMBIE, ENEMY_BUG } enemy_kind_t;

typedef enum {
  BUG_BEETLE,
  BUG_FLY,
  BUG_BEE,
  BUG_DARK,
  BUG_SMALL,
  BUG_VARIANTS
} bug_variant_t;

typedef struct {
  bool alive;
  enemy_kind_t kind;
  int variant;
  float x, y, vx, vy;
  float w, h;
  int facing;
  float speed;
  bool grounded;
  float hop_cooldown;
  float phase;
  float anim_time;
  float base_y;
} enemy_t;

typedef struct {
  bool alive;
  item_kind_t kind;
  float x, y, vx, vy; /* centre */
  float age;
} pickup_t;

typedef struct {
  bool alive;
  float x, y, vx, life;
} bullet_t;

typedef struct {
  float x0, x1, y, age;
  int facing;
  bool alive;
} ray_t;

typedef enum {
  FX_ZOMBIE_BOOM,
  FX_ZOMBIE_GREEN,
  FX_BUG_BOOM,
  FX_BUG_BLUE,
  FX_SMOKE,
  FX_REMAINS,
  FX_MUZZLE,
  FX_BUG_SPLAT
} fx_kind_t;

typedef struct {
  bool alive;
  fx_kind_t kind;
  float x, y, age, duration;
  int facing;
} effect_t;

typedef struct {
  bool alive;
  float x, y, vx, vy, age, life, size;
  uint8_t r, g, b;
} particle_t;

typedef struct {
  int col, row;
  float age;
} bump_t;

typedef struct {
  level_t level;
  player_t player;
  enemy_t enemies[MAX_ENEMIES];
  pickup_t pickups[MAX_PICKUPS];
  bool item_taken[MAX_LEVEL_ITEMS];
  bullet_t bullets[MAX_BULLETS];
  ray_t rays[MAX_RAYS];
  effect_t effects[MAX_EFFECTS];
  particle_t particles[MAX_PARTICLES];
  bump_t bumps[MAX_BUMPS];
  event_t events[MAX_EVENTS];
  int event_count;

  rng_t rng;
  game_state_t state;
  float time;
  float state_time;
  float camera_x;
  float view_w; /* the window's width in world units; set by the caller */
  float zombie_timer, bug_timer;
  bool spawning;

  int score;
  int coins;
  int zombies_killed;
  int bugs_killed;
} world_t;

void world_init(world_t* world, uint32_t seed);
void world_update(world_t* world, const controls_t* controls, float dt);
/* 0 at the start of the level, 1 at the castle. */
float world_progress(const world_t* world);
void world_clear_events(world_t* world);
bool world_has_event(const world_t* world, event_kind_t kind);
int world_count_enemies(const world_t* world, enemy_kind_t kind);
rect_t player_rect(const player_t* player);
rect_t enemy_rect(const enemy_t* enemy);
void world_emit(world_t* world, event_kind_t kind, float x);

/* physics.c */
typedef struct {
  bool grounded;
  bool hit_left, hit_right, hit_ceiling;
  int ceiling_col, ceiling_row;
} move_result_t;

/* Moves a box (middle x, soles y, size w by h) by its velocity against the
 * tiles, zeroing velocity components that hit something. */
move_result_t physics_move(const level_t* level, float* x, float* y,
                           float* vx, float* vy, float w, float h, float dt);

/* player.c */
void player_reset(player_t* player, const level_t* level);
void player_update(world_t* world, const controls_t* controls, float dt);
void player_hurt(world_t* world, float from_x);

/* enemy.c */
void enemies_update(world_t* world, float dt);
void spawner_update(world_t* world, float dt);
void spawn_zombie(world_t* world, float x, float y);
void spawn_bug(world_t* world, int variant, float x, float y);

/* combat.c */
void combat_update(world_t* world, float dt);
void world_fire_bullet(world_t* world);
void world_cast_spell(world_t* world);
void world_kill_enemy(world_t* world, enemy_t* enemy, bool by_spell);
void world_add_effect(world_t* world, fx_kind_t kind, float x, float y,
                      float duration, int facing);
void world_burst(world_t* world, float x, float y, int count, uint8_t r,
                 uint8_t g, uint8_t b, float speed);

/* items.c */
void items_update(world_t* world, float dt);
void world_bump_block(world_t* world, int col, int row);
void world_spawn_pickup(world_t* world, item_kind_t kind, float x, float y);

#endif  // GAME_SIM_WORLD_H_
