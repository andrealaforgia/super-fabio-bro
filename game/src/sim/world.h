/**
 * @file world.h
 * @brief The whole game state and its rules, with no drawing and no platform code:
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

#include "buttons.h"
#include "event_system.h"
#include "object_pool.h"
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
#define MAX_POPUPS 24
#define MAX_BANANAS 8
#define MAX_SKEWERS 8
#define MAX_DEBRIS 128 /* room for a gorilla's and a chef's showers at once */
#define BANANA_SHOWER 50
#define MAX_FIREBALLS 8
#define MAX_DRUM_WORDS 24
#define POPUP_SECONDS 1.1f
#define POPUP_RISE 70.0f /* how far the points climb before they are gone */

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
#define PISTOL_AMMO 15 /* bullets each pistol adds */
/* The soap bubble shotgun: each shot (BUBBLE_GUN_AMMO to a pickup, one every
 * BUBBLE_COOLDOWN while the key is held) lets BUBBLES_PER_SHOT bubbles go in a
 * loose spread. A bubble swells all the way from BUBBLE_R0 to BUBBLE_R_MAX as it
 * flies; the first enemy it meets is wrapped in it, floats helpless and, after
 * BUBBLE_WRAP_SECONDS, bursts with the bubble. */
#define BUBBLE_GUN_AMMO 50
#define BUBBLES_PER_SHOT 2
#define BUBBLE_COOLDOWN 0.12f
#define BUBBLE_SPEED 330.0f
#define BUBBLE_SPREAD 14.0f /* degrees either side of level */
#define BUBBLE_R0 5.0f
#define BUBBLE_R_MAX 48.0f
#define BUBBLE_GROW_SECONDS 1.6f
#define BUBBLE_LIFE 1.8f
#define BUBBLE_WRAP_SECONDS 1.5f
#define BUBBLE_LIFT 28.0f /* how fast a wrapped enemy floats up, in pixels a second */
/* The green lightsaber turns Fabio into a Jedi knight for JEDI_SECONDS. The shoot key swings it
 * and sends a green burst flying ahead that destroys every enemy it passes through, until it
 * has gone FORCE_BURST_SPEED * FORCE_BURST_LIFE or meets a wall. The old knight's face fades in
 * at the top left to wish him well for GREETING_SECONDS: in, then still, then out. */
#define JEDI_SECONDS 20.0f
#define JEDI_COOLDOWN 0.4f
#define FORCE_BURST_SPEED 640.0f
#define FORCE_BURST_LIFE 0.9f
#define FORCE_BURST_W 44.0f
#define FORCE_BURST_H 70.0f
#define GREETING_SECONDS 4.5f
#define GREETING_FADE_IN 0.5f
#define GREETING_FADE_OUT 0.8f
#define MAX_FORCE_BURSTS 12
/* Fabio's three friends. A sushi dumpling brings Andrea, a Chinese dragon Alessandro and boxing
 * gloves Giulia. Each stays ALLY_SECONDS, a few steps ahead of Fabio (each a different number),
 * and fights by himself: Andrea strokes his belly, a wobble that kills everything round him;
 * Alessandro and Giulia run at what comes within their reach and strike it. Nothing hurts them.
 * A water polo ball brings a fourth, Giacomo, who keeps his distance and throws the ball at what
 * comes up ahead of him. Put him next to Giulia and she punches him, every time she reaches him:
 * he sits on the ground with a black eye for five seconds, then gets up (and is punched again if
 * she is still on him). */
#define ALLY_SECONDS 30.0f
#define ALLY_W 26.0f
#define ALLY_H 54.0f
#define ALLY_ARRIVE_SECONDS 0.35f
#define MAX_ALLIES 4
#define GIACOMO_SIT_SECONDS 5.0f /* on the ground after Giulia's punch, before he gets up and carries on */
#define MAX_POLO_BALLS 8
#define POLO_BALL_SPEED 700.0f
#define POLO_BALL_LIFE 1.2f
#define POLO_BALL_R 12.0f
#define SPELL_LENGTH 620.0f
#define BULLET_SPEED 760.0f
#define HELICOPTER_FUEL_SECONDS 30.0f
#define HELICOPTER_W 84.0f /* the box he lands in, and the one he flies in */
#define HELICOPTER_H 60.0f
#define HELICOPTER_SPEED 264.0f /* up and down */
#define HELICOPTER_SPEEDUP 1.1f /* his walking and running speeds, sideways, in it */
#define HELICOPTER_CEILING 140.0f /* the highest his skids go: under the scoreboard */
#define HELICOPTER_BOOM_SECONDS 0.9f /* seven frames */
/* Jumping out of the helicopter (jump while holding down) opens a parachute, which drops him
 * slowly and gently, lets him steer, shoot and land. */
#define PARACHUTE_FALL 115.0f      /* the most he falls per second under it */
#define PARACHUTE_DRIFT 0.75f      /* the share of his walking speed he can steer at */
#define PARACHUTE_OPEN_SECONDS 0.3f
#define PARACHUTE_LANDING_SECONDS 0.5f
#define PARACHUTE_REMAINS_SECONDS 2.0f
/* Fabio's drum kit. He jumps into it and plays by himself for DRUM_SECONDS, as
 * untouchable as in the helicopter, and the kit shouts a word every
 * DRUM_WORD_EVERY seconds: it swells into a ring DRUM_WORD_RADIUS wide over
 * DRUM_WORD_SECONDS, then fades, and kills any enemy inside it. */
#define DRUM_SECONDS 10.0f
#define DRUM_W 70.0f /* the box he jumps into */
#define DRUM_H 62.0f
#define DRUM_FRAME_SECONDS 0.05f /* eight frames of playing, flashing by */
#define DRUM_FRAMES 8
#define DRUM_WORDS 6
#define DRUM_WORD_EVERY 0.2f
#define DRUM_WORD_SECONDS 1.2f
#define DRUM_WORD_RADIUS 320.0f
#define PICKUP_SIZE 24.0f /* the box a prize from a block falls and rests as */

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
  EVENT_BLOCK_OPENS, /* a block opens and an item comes up out of it */
  EVENT_DEATH,
  EVENT_WIN,
  EVENT_HEAL,
  EVENT_BOOK_READER_DIES,
  EVENT_MONK_DIES,
  EVENT_CHEF_DIES,
  EVENT_GORILLA_DIES,
  EVENT_BANANA_THROWN,
  EVENT_DEMON_DIES,
  EVENT_DEMON_CASTS,
  EVENT_HELICOPTER_BOARDS,
  EVENT_HELICOPTER_EXPLODES,
  EVENT_SKEWER_THROWN,
  EVENT_BUBBLE_SHOT,
  EVENT_BUBBLE_WRAPS,
  EVENT_BUBBLE_POPS,
  EVENT_ALLY_ARRIVES,
  EVENT_ALLY_BELLY,  /* Andrea strokes his belly */
  EVENT_ALLY_KUNGFU, /* Alessandro strikes */
  EVENT_ALLY_PUNCH,  /* Giulia punches */
  EVENT_ALLY_THROW,  /* Giacomo throws the ball */
  EVENT_ALLY_OUCH,   /* Giacomo is punched */
  EVENT_LIGHTSABER,
  EVENT_FORCE_BURST,
  EVENT_SLOP_THROWN,
  EVENT_WARP,
  EVENT_SLOP_SPEAKS,   /* his robot voice: "Certainly! Here is some slop." */
  EVENT_GORILLA_GROWLS, /* as his roar begins */
  EVENT_SLOP_DIES,
  EVENT_DRUMS_START,
  /* The six words, in the order of kDrumWords: EVENT_DRUM_BOOM + the word. */
  EVENT_DRUM_BOOM,
  EVENT_DRUM_BADABOOM,
  EVENT_DRUM_TASH,
  EVENT_DRUM_BODOBOOOM,
  EVENT_DRUM_BOING,
  EVENT_DRUM_DISH,
  EVENT_PARACHUTE_OPENS,
  EVENT_VASE_SMASHES,
  EVENT_WATER_ON,
  EVENT_BULL_CHARGES,
  EVENT_KIND_COUNT
} event_kind_t;

typedef struct {
  float x, y, vx, vy;
  int facing; /* +1 right, -1 left */
  bool grounded;
  bool alive;
  int hp;
  float invulnerable;
  float hurt_timer;
  int ammo; /* bullets left; the pistol is in hand while there are any */
  float jedi_time; /* seconds left as a Jedi knight, with the lightsaber */
  int bubbles; /* shots left in the soap bubble shotgun, which takes the pistol's place while there are any */
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
  int platform; /* the plank he stands on (an index into the level's), or -1 */
  bool flying;  /* he is in the helicopter */
  float fuel;   /* seconds of flight left */
  bool chute;       /* he is coming down under the parachute */
  float chute_age;  /* seconds since it opened */
  float landing;    /* seconds left of the touchdown, the canopy collapsing round him */
  float tap_window; /* seconds since the last jump tap in the helicopter: a second one bails out */
} player_t;

/* The book reader is no enemy: he reads Fabio's book, laughs and does no harm.
 * Only a shot (a bullet or the ray) sets him off. */
typedef enum {
  ENEMY_ZOMBIE,
  ENEMY_BUG,
  ENEMY_BOOK_READER,
  ENEMY_MONK,
  ENEMY_CHEF,
  ENEMY_GORILLA,
  ENEMY_DEMON,
  ENEMY_SLOP
} enemy_kind_t;

typedef enum {
  BUG_BEETLE,
  BUG_FLY,
  BUG_BEE,
  BUG_DARK,
  BUG_SMALL,
  BUG_VARIANTS
} bug_variant_t;

typedef struct {
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
  float phase_seed; /* de-synchronises the zombies' floating speech */
  int leap;         /* -1 or 1 while climbing out of a pit that way, otherwise 0 */
} enemy_t;

typedef struct {
  item_kind_t kind;
  float x, y, vx, vy; /* centre */
  float age;
} pickup_t;

typedef struct {
  float x, y, vx, life;
} bullet_t;

/* A lamb skewer Fabio shoots from the helicopter. It leaves at SKEWER_ANGLE
 * degrees above the horizontal and arcs under SKEWER_GRAVITY; the sprite turns
 * with it: 45 degrees, flat at the top, then another 80, stopping 10 short of straight down. */
#define SKEWER_SPEED 560.0f
#define SKEWER_ANGLE 45.0f
#define SKEWER_DIVE 80.0f
#define SKEWER_GRAVITY 900.0f
#define SKEWER_LIFE 1.4f
#define SKEWER_COOLDOWN 0.28f
#define SKEWER_SIZE 24.0f /* the box that hits */
typedef struct {
  float x, y, vx, vy, age;
} skewer_t;

/* Degrees above flat: 45 at the muzzle, 0 at the top of the arc, then on to
 * -SKEWER_DIVE, 10 degrees short of straight down, by the time it falls as fast as it left. */
float skewer_tilt(const skewer_t* skewer);

/* A soap bubble in flight (`victim` is -1) or wrapped round an enemy (`victim` is
 * his slot in the enemy pool, `victim_kind` what he was, `wrapped` seconds since). */
typedef struct {
  float x, y, vx, vy, age, r; /* centre and radius */
  int victim;
  int victim_kind;
  float wrapped;
} bubble_t;
#define MAX_BUBBLES 64

typedef enum { ALLY_ANDREA, ALLY_ALESSANDRO, ALLY_GIULIA, ALLY_GIACOMO, ALLY_KIND_COUNT } ally_kind_t;
typedef struct {
  ally_kind_t kind;
  float x, y, vx, vy; /* the middle of the feet */
  int facing;
  bool grounded;
  float left;     /* seconds still to stay */
  float age;      /* seconds since he arrived */
  float attack;   /* seconds into a strike, 0 when not striking */
  float cooldown; /* before the next strike */
  float anim;
  bool struck;    /* this strike has landed */
  int platform;   /* the plank he stands on (a balcony, a roof, a capsule), -1 for none */
  bool sitting;   /* Giacomo, punched by Giulia: on the ground with a black eye */
  bool crouching; /* Giacomo is throwing low, at something crawling on the ground */
  float sit_left; /* seconds still to sit on the ground after the punch */
} ally_t;

/* A water polo ball in flight: it kills the first enemy it meets. */
typedef struct {
  float x, y, vx, age; /* centre */
} polo_ball_t;

/* A green burst from the lightsaber: flies on through everything it destroys. */
typedef struct {
  float x, y, vx, age; /* centre */
} force_burst_t;

/* A banana the gorilla threw: it spins along an arc and kills whoever it hits
 * first, but never the gorilla who threw it; Fabio only loses a heart. */
typedef struct {
  float x, y, vx, vy, age; /* centre */
  int thrower;             /* the gorilla's slot in the enemy pool */
  bool slop;               /* a ball of AI slop from the slop monster: it hurts Fabio and
                            * splats against anything, but hits no other enemy */
} banana_t;

/* Harmless pieces flung every way out of a burst character: bananas out of the
 * gorilla, vegetables out of the chef. They go through everything, spin, slow
 * down and fade out over their `life`, growing to `grow` times their size. */
typedef enum { DEBRIS_BANANA, DEBRIS_VEGETABLE, DEBRIS_SLOP } debris_kind_t;
#define SLOP_PIECES 6 /* the different blobs of a burst slop monster */
#define SLOP_SHOWER 30
#define VEGETABLE_KINDS 16
#define VEGETABLE_SHOWER 50
#define VEGETABLE_GROWTH 2.5f
typedef struct {
  debris_kind_t kind;
  int variant; /* which vegetable */
  float x, y, vx, vy, age, life, grow, spin;
} debris_t;

/* The demon's spell: a ball of fire flying straight at where Fabio was. It
 * costs Fabio a heart and goes out against a wall; it passes everybody else. */
typedef struct {
  float x, y, vx, vy, age; /* centre */
} fireball_t;

typedef struct {
  float x0, x1, y, age;
  int facing;
} ray_t;

typedef enum {
  FX_ZOMBIE_BOOM,
  FX_ZOMBIE_GREEN,
  FX_BUG_BOOM,
  FX_BUG_BLUE,
  FX_SMOKE,
  FX_REMAINS,
  FX_MUZZLE,
  FX_BUG_SPLAT,
  FX_BOOK_READER_BOOM,
  FX_MONK_BOOM,
  FX_SKELETON_VANISH,
  FX_CHEF_EXPLODE,
  FX_DEMON_DEATH,
  FX_HELICOPTER_BOOM,
  FX_ALLY_POOF,  /* an ally arriving or leaving, 6 frames */
  FX_ALLY_WAVE,  /* Andrea's belly wobble, 4 frames */
  FX_ALLY_SLASH, /* Alessandro's chi slash, 4 frames */
  FX_ALLY_PUNCH, /* Giulia's punch burst, 4 frames */
  FX_FORCE_HIT, /* a green burst meeting an enemy, 6 frames */
  FX_SLOP_DEATH, /* the slop monster tearing into noise and dripping away, 8 frames */
  FX_SLOP_SPLAT, /* a ball of slop landing, 6 frames */
  FX_CHUTE_REMAINS /* the crumpled parachute he leaves on the ground */
} fx_kind_t;

typedef struct {
  fx_kind_t kind;
  float x, y, age, duration;
  int facing;
} effect_t;

typedef struct {
  float x, y, vx, vy, age, life, size;
  uint8_t r, g, b;
} particle_t;

typedef struct {
  int col, row;
  float age;
} bump_t;

/* The points just scored, drifting up from where they were earned. */
typedef struct {
  float x, y, age;
  int points;
} popup_t;

/* Where a plank is now and how far it moved in the last update. */
typedef struct {
  float x, top; /* middle of the top */
  float dx, dy;
  bool off; /* not there: nobody can stand on it */
} plank_t;

/* The one helicopter, resting on the grass until Fabio drops into it. Once he
 * has, it is no longer there to land on: it is the one he flies. */
typedef struct {
  bool present;
  float x, y; /* middle, and the skids */
} helicopter_t;

/* A terracotta vase on a balcony: Fabio knocks it off as he walks by, and it drops on whatever is
 * below. It is a vase once only. */
typedef enum { VASE_STANDING, VASE_FALLING, VASE_SMASHED } vase_state_t;
typedef struct {
  vase_state_t state;
  float x, y; /* the middle of its base while it falls */
  float vy;
} vase_t;
/* The Bullring's bull: walk up to the statue and it comes alive, rears, then charges the other
 * way from Fabio, trampling every enemy in its path. It never hurts him. */
typedef struct {
  bool active;
  bool spent;   /* it has charged, and the statue is quiet until he has walked away from it */
  float x;      /* the middle */
  float start;  /* where it began */
  float age;
  int dir;
} bull_t;
#define BULL_TRIGGER 90.0f        /* how near to the statue he has to come */
#define BULL_REAR_SECONDS 0.6f
#define BULL_SPEED 620.0f
#define BULL_RUN_DISTANCE 2600.0f
#define BULL_W 150.0f
#define BULL_H 84.0f
#define BULL_REARM_DISTANCE 700.0f
#define VASE_W 24.0f
#define VASE_H 30.0f
#define VASE_KNOCK_RANGE 44.0f /* how near he has to walk, along the balcony */

/* The aqueduct's tap: with the water on, curtains fall from its arches for WATER_SECONDS and
 * whatever is under one is washed away. */
#define WATER_SECONDS 14.0f
#define WATER_HALF_WIDTH 24.0f
#define TAP_RANGE 90.0f

/* One of Fabio's drum kits, standing on the grass. Every jump into one starts a fresh gig. */
typedef struct {
  bool armed;     /* he has left it since the last gig, so jumping in starts another */
  float x, y;     /* middle, and the base */
  float playing;  /* seconds of the gig left, 0 when nobody is at it */
  float next_word;
  int last_word;
} drum_kit_t;

/* A word the kit has shouted: a ring growing round the kit, then fading. */
typedef struct {
  float x, y; /* where the kit was */
  float age;
  int word; /* an index into kDrumWords */
} drum_word_t;

/* Warp pipes. Standing on the cap of one and tapping down, Fabio sinks into it
 * (WARP_SINK_SECONDS), the world swaps to the cave under it (or back to the level above)
 * behind a fade, and he rises out of the pipe at the other end (WARP_RISE_SECONDS).
 * What the main level looked like is kept in the stash while he is in a cave. */
#define WARP_SINK_SECONDS 0.55f
#define WARP_RISE_SECONDS 0.55f
typedef enum { WARP_NONE, WARP_SINKING, WARP_RISING } warp_phase_t;
typedef struct {
  warp_phase_t phase;
  float t;  /* seconds into the phase */
  int pipe; /* the pipe he went into, an index into the level's warps */
} warp_t;
typedef struct {
  level_t level;
  bool item_taken[MAX_LEVEL_ITEMS];
  helicopter_t helicopter;
  drum_kit_t drums[MAX_DRUM_KITS];
  int drum_count;
  float camera_x;
  bool spawning;
} warp_stash_t;

typedef struct {
  level_t level;
  warp_t warp;
  warp_stash_t stash;           /* the main level, while he is in a cave */
  bool warp_used[MAX_WARPS];    /* pipes of the main level that have been gone down */
  int warp_from;                /* the main level's pipe the cave hangs under */
  helicopter_t helicopter;
  drum_kit_t drums[MAX_DRUM_KITS];
  int drum_count;
  plank_t planks[MAX_LEVEL_PLATFORMS];
  player_t player;
  /* Everything that comes and goes lives in an engine object pool. */
  object_pool_t enemies;   /* enemy_t */
  object_pool_t pickups;   /* pickup_t */
  object_pool_t bubbles;   /* bubble_t */
  object_pool_t allies;    /* ally_t */
  object_pool_t force_bursts; /* force_burst_t */
  object_pool_t polo_balls;   /* polo_ball_t */
  bool item_taken[MAX_LEVEL_ITEMS];
  object_pool_t bullets;   /* bullet_t */
  object_pool_t rays;      /* ray_t */
  object_pool_t effects;   /* effect_t */
  object_pool_t particles; /* particle_t */
  object_pool_t bumps;     /* bump_t */
  object_pool_t popups;    /* popup_t */
  object_pool_t bananas;   /* banana_t */
  object_pool_t skewers;   /* skewer_t */
  object_pool_t fireballs; /* fireball_t */
  object_pool_t debris;    /* debris_t */
  object_pool_t drum_words; /* drum_word_t */
  vase_t vases[MAX_VASES];  /* in the level's order */
  bull_t bull;
  float water_left;         /* seconds of water still falling, 0 when the tap is off */
  event_system_ptr events; /* where the world announces what happens; may be NULL */

  uint32_t seed; /* the level was generated from this */
  rng_t rng;
  game_state_t state;
  float time;
  float state_time;
  float camera_x;
  float view_w; /* the window's width in world units; set by the caller */
  float zombie_timer, bug_timer, book_reader_timer, monk_timer, chef_timer, gorilla_timer, slop_timer;
  float arrival_wait, special_wait; /* until the next arrival, and the next special one */
  bool spawning;

  int score;
  int coins;
  int zombies_killed;
  int bugs_killed;
  int book_readers_killed;
  int monks_killed;
  int chefs_killed;
  int gorillas_killed;
  float greeting_left; /* seconds left of the old knight's greeting at the top left, 0 for none */
  int slops_killed;
  int demons_killed;
} world_t;

/* A new world with its object pools, or NULL when memory runs out.
 * `events` is where the world publishes what happens (event_kind_t as the
 * type, the x position as a float payload); the caller owns it, and it may be
 * NULL. */
world_t* world_new(uint32_t seed, event_system_ptr events);
/* Starts the level again in the same world, keeping its pools and events. */
void world_restart(world_t* world, uint32_t seed);
void world_free(world_t* world);
void world_update(world_t* world, const controls_t* controls, float dt);
/* 0 at the start of the level, 1 at the castle. */
float world_progress(const world_t* world);
int world_count_enemies(const world_t* world, enemy_kind_t kind);
frect_t player_rect(const player_t* player);
frect_t enemy_rect(const enemy_t* enemy);
frect_t helicopter_rect(const helicopter_t* helicopter);
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

/* platforms.c */
#define PLANK_THICKNESS 14.0f
/* Puts every plank where it is at world->time and works out how far it moved. */
void platforms_update(world_t* world);
/* The same, without any movement: for a level that has just begun. */
void platforms_reset(world_t* world);
/* While Fabio stands on a plank it carries him along. */
void platforms_carry(world_t* world, player_t* player);
/* The same for anyone with a position: Fabio's friends ride the planks too. */
void platforms_carry_body(world_t* world, float* x, float* y, int* platform);
void platforms_land_body(world_t* world, float* x, float* y, float* vy, bool* grounded, int* platform, float feet_before);
/* After he has moved: lands him on a plank he came down onto, or lets him go. */
void platforms_land(world_t* world, player_t* player, float feet_before);

/* player.c */
void player_reset(player_t* player, const level_t* level);
void player_update(world_t* world, const controls_t* controls, float dt);
void player_hurt(world_t* world, float from_x);

/* enemy.c */
/* A walker that falls into a pit leaps out again, towards the nearer side, and
 * every so often one is sent up out of a pit ahead of Fabio. */
#define PIT_LEAP_LINE ((float)(GROUND_ROW * TILE) + 56.0f)
#define PIT_LEAP_SPEED 190.0f
#define PIT_EXIT_REACH 5     /* columns it will look for a way out */
#define PIT_EXIT_HEIGHT 5    /* tiles of wall it can leap onto, from the ground up */
#define PIT_AMBUSH_CHANCE 0.3f /* of a walker's arrivals: up out of a pit ahead */

void enemies_update(world_t* world, float dt);
void spawner_update(world_t* world, float dt);
/* Arrivals are spread out: one at a time, ARRIVAL_GAP_START seconds apart at
 * the start of the level down to ARRIVAL_GAP_END near the castle (plus up to
 * ARRIVAL_GAP_JITTER), never while CROWD_START (rising to CROWD_END) are
 * already within a screen of Fabio. Book readers, monks, chefs and gorillas
 * come at least SPECIAL_GAP apart, no more than SPECIALS_MAX about at once. */
/* How fast characters come: every interval below and every kind's own interval
 * is divided by this. 1 is the calm base rate; 2 is twice as many. */
#define SPAWN_PACE 2.0f
#define ARRIVAL_GAP_START (4.5f / SPAWN_PACE)
#define ARRIVAL_GAP_END (2.8f / SPAWN_PACE)
#define ARRIVAL_GAP_JITTER (1.5f / SPAWN_PACE)
#define CROWD_START 6
#define CROWD_END 9
#define SPECIAL_GAP (12.0f / SPAWN_PACE)
#define SPECIALS_MAX 2
/* How many may be round Fabio before the next one is held back. */
int world_crowd_limit(const world_t* world);
void spawn_zombie(world_t* world, float x, float y);
void spawn_bug(world_t* world, int variant, float x, float y);
void spawn_book_reader(world_t* world, float x, float y);
/* The ones who will not pair, and the one who loves PRs. The monk stands where
 * he appears; the skeleton walks slowly towards Fabio. Either hurts Fabio when
 * he walks into them, like a zombie, and he can stomp, shoot or swing at them
 * in return: the monk explodes, the skeleton vanishes. */
typedef enum { MONK_HOODED, MONK_SKELETON, MONK_VARIANTS } monk_variant_t;
void spawn_monk(world_t* world, monk_variant_t variant, float x, float y);
#define MONK_MAX 2
/* The hooded monk starts to show his cross when Fabio is this close. */
#define MONK_CROSS_RANGE 320.0f
/* The chef offers elephant carpaccio. He stands where he appeared, smiling
 * while Fabio is far. Once Fabio is within CHEF_ATTACK_RANGE he swings his knife, and the
 * swing hurts for as long as the blade is out in front of him. Stomp, shoot or
 * swing at the chef and he explodes. */
#define CHEF_MAX 1
/* Horizontal space kept between a new book reader, monk or chef and any other one. */
#define CHARACTER_SPACING 130.0f
#define CHEF_ATTACK_RANGE 150.0f
#define CHEF_KNIFE_REACH 64.8f
#define CHEF_SWING_FRAMES 4
#define CHEF_SWING_FPS 9.0f
#define CHEF_EXPLODE_SECONDS 0.6f
/* He only appears on flat grass with this many tiles of open air above. */
#define CHEF_HEIGHT_TILES 3
void spawn_chef(world_t* world, float x, float y);
/* Whether a chef at (x, y) fits on flat grass with nothing in the way above. */
bool chef_has_room(const world_t* world, float x, float y);
bool chef_attacking(const enemy_t* chef);
/* The frame of the swing, 0 to CHEF_SWING_FRAMES - 1; the knife is out in front
 * of him from the middle of the swing. */
int chef_swing_frame(const enemy_t* chef);
bool chef_knife_out(const enemy_t* chef);
/* The gorilla stands where he appears, always facing Fabio, and goes round and
 * round: he stands, roars, beats his chest, then throws a banana at Fabio
 * whenever he is on screen. Touching him hurts like a zombie; a stomp, a
 * bullet, the ray or the pickaxe kills him. One at a time. */
#define GORILLA_MAX 1
/* The AI slop monster stands where he appears, turned towards Fabio, and goes round
 * and round: he stands dripping, winds up a ball of slop over his head, and hurls
 * it at Fabio (anywhere BANANA_MISS either side of him) like the gorilla's banana. The
 * ball splats against whatever it meets, and costs Fabio a heart. */
#define SLOP_MAX 1
#define SLOP_STAND_SECONDS 1.5f
#define SLOP_WINDUP_SECONDS 0.64f /* four frames */
#define SLOP_THROW_SECONDS 0.6f   /* four frames */
#define SLOP_RELEASE 0.3f         /* into the throw, the ball leaves his hand */
#define SLOP_CYCLE (SLOP_STAND_SECONDS + SLOP_WINDUP_SECONDS + SLOP_THROW_SECONDS)
typedef enum { SLOP_STANDING, SLOP_WINDING_UP, SLOP_THROWING } slop_act_t;
slop_act_t slop_act(const enemy_t* slop, float* into);
void spawn_slop(world_t* world, float x, float y);
#define GORILLA_STAND_SECONDS 1.4f
#define GORILLA_ROAR_SECONDS 0.64f  /* four frames */
#define GORILLA_BEAT_SECONDS 0.72f  /* two frames, back and forth */
#define GORILLA_THROW_SECONDS 0.75f /* four frames, the last held */
#define GORILLA_RELEASE 0.45f       /* into the throw, the banana leaves his hand */
#define GORILLA_CYCLE \
  (GORILLA_STAND_SECONDS + GORILLA_ROAR_SECONDS + GORILLA_BEAT_SECONDS + GORILLA_THROW_SECONDS)
#define BANANA_GRAVITY 900.0f
#define BANANA_MISS 90.0f /* he aims anywhere this far either side of Fabio */
typedef enum { GORILLA_STANDING, GORILLA_ROARING, GORILLA_BEATING, GORILLA_THROWING } gorilla_act_t;
/* What he is doing now, and for how many seconds he has been doing it. */
gorilla_act_t gorilla_act(const enemy_t* gorilla, float* into);
void spawn_gorilla(world_t* world, float x, float y);
/* When the hooded monk goes up in smoke, a demon rises where he stood once the
 * blast has thinned to smoke (DEMON_RISE_DELAY into it), fading in over
 * DEMON_FADE_SECONDS as the monk's smoke fades out; until he is all there,
 * walking into him does nothing. He stands there, always facing
 * Fabio, and every so often casts a fireball at him while he is on screen.
 * Touching him hurts; a stomp, a bullet, the ray or the pickaxe kills him, and
 * he dissolves into smoke. He is not one of the characters the spawner brings. */
#define MONK_BOOM_SECONDS 1.75f
#define DEMON_RISE_DELAY 0.6f
#define DEMON_FADE_SECONDS (MONK_BOOM_SECONDS - DEMON_RISE_DELAY)
#define DEMON_STAND_SECONDS 2.2f
#define DEMON_CAST_SECONDS 0.8f  /* four frames */
#define DEMON_RELEASE 0.4f       /* into the cast, the fireball leaves his fist */
#define DEMON_CYCLE (DEMON_STAND_SECONDS + DEMON_CAST_SECONDS)
#define DEMON_DEATH_SECONDS 1.0f /* eight frames */
#define FIREBALL_SPEED 330.0f
void spawn_demon(world_t* world, float x, float y);
bool demon_casting(const enemy_t* demon, float* into);
/* How much of him is there: 0 as he starts to rise, 1 once he has faded in. */
float demon_presence(const enemy_t* demon);
/* They come every 20 to 35 seconds (divided by SPAWN_PACE), ahead of Fabio, and stay where they
 * appear: never more than this many at once. */
#define BOOK_READER_MAX 2
/* Each reads, laughs, reads again, laughs again, whatever Fabio does: a loop
 * of BOOK_READER_READ_SECONDS then BOOK_READER_LAUGH_SECONDS, from anim_time. */
#define BOOK_READER_READ_SECONDS 3.0f
#define BOOK_READER_LAUGH_SECONDS 2.4f
typedef enum { BOOK_READER_READING, BOOK_READER_LAUGHING } book_reader_mood_t;
book_reader_mood_t book_reader_mood(const enemy_t* book_reader);

/* city.c: the vases and the aqueduct */
void city_update(world_t* world, const controls_t* controls, float dt);
/* Is the water running, and is Fabio standing at the tap? */
bool water_running(const world_t* world);
bool fabio_at_tap(const world_t* world);
/* helicopter.c */
/* Fabio lands on the resting helicopter from above and flies it, jump or up
 * to climb, down to sink, nothing to hover, until the fuel is gone after
 * HELICOPTER_FUEL_SECONDS: then it explodes and he falls, unharmed. */
void helicopter_update(world_t* world, float dt);
/* Jump out of the helicopter: a parachute opens if he is up in the air. */
void helicopter_bail_out(world_t* world);

/* drums.c */
extern const char* const kDrumWords[DRUM_WORDS];
frect_t drum_kit_rect(const drum_kit_t* kit);
/* How far the ring of a word `age` seconds old has swollen. */
float drum_word_radius(float age);
/* Whether he is sitting at a kit. */
bool world_drumming(const world_t* world);
/* Fabio jumps into a kit, and it plays for DRUM_SECONDS. */
void drums_update(world_t* world, const controls_t* controls, float dt);

/* combat.c */
void combat_update(world_t* world, float dt);
void world_fire_bullet(world_t* world);
/* From the helicopter only: a skewer on a parabola. Never runs out. */
void world_fire_skewer(world_t* world);
void world_cast_spell(world_t* world);
void world_kill_enemy(world_t* world, enemy_t* enemy, bool by_spell);
/* The gorilla throws a banana on an arc that comes down on Fabio, from wherever
 * round him it starts in the open. */
void world_throw_banana(world_t* world, const enemy_t* gorilla);
/* Flings `count` harmless pieces of `kind` out of (x, y), evenly round the circle. */
void world_fling(world_t* world, debris_kind_t kind, float x, float y, int count);
/* The demon casts a fireball from his fist straight at Fabio. */
void world_cast_fireball(world_t* world, const enemy_t* demon);
void world_add_effect(world_t* world, fx_kind_t kind, float x, float y,
                      float duration, int facing);
/* Adds `points` to the score and floats them up from (x, y). */
void world_award(world_t* world, int points, float x, float y);
void world_burst(world_t* world, float x, float y, int count, uint8_t r,
                 uint8_t g, uint8_t b, float speed);

/* items.c */
void items_update(world_t* world, float dt);
void world_bump_block(world_t* world, int col, int row);
void world_spawn_pickup(world_t* world, item_kind_t kind, float x, float y);

/* One shot of the soap bubble shotgun: BUBBLES_PER_SHOT bubbles leave the muzzle. */
void world_fire_bubbles(world_t* world);
void bubbles_update(world_t* world, float dt);
/* jedi.c: the lightsaber's green bursts, and the old knight's greeting. */
void world_fire_force_burst(world_t* world);
void force_bursts_update(world_t* world, float dt);
/* How solid the greeting is `left` seconds before it ends: 0 at both ends, 1 in the middle. */
float greeting_alpha(float left);

/* warps.c: the warp pipes. */
void warps_update(world_t* world, const controls_t* controls, float dt);
/* The pipe Fabio stands on the cap of, or -1. */
int warp_pipe_under_fabio(const world_t* world);
/* How black the screen is as the warp goes through: 0 clear, 1 black. */
float warp_fade(const world_t* world);
/* Everything that comes and goes (enemies, bullets, effects ...) is cleared. */
void world_clear_actors(world_t* world);

/* allies.c: Fabio's friends. */
/* Calls the ally in (or, if he is already here, keeps him another ALLY_SECONDS). */
void world_summon_ally(world_t* world, ally_kind_t kind);
void allies_update(world_t* world, float dt);
void polo_balls_update(world_t* world, float dt);
/* Brings them all to Fabio's side: he has been through a pipe. */
void allies_regroup(world_t* world);
/* How far ahead of Fabio the ally stands, how long a strike takes. */
/* The friend a pickup brings. */
ally_kind_t ally_for_item(item_kind_t kind);
float ally_offset(ally_kind_t kind);
float ally_attack_seconds(ally_kind_t kind);

/* Whether a bubble has this enemy wrapped: he floats, harmless, until it bursts. */
bool enemy_in_bubble(const world_t* world, const enemy_t* enemy);

#endif  // GAME_SIM_WORLD_H_
