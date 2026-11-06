/**
 * @file pose.h
 * @brief Which sprite to draw for a character right now. Every sprite the
 * game uses is named here, as a strip of frames (count 0 is a single image
 * named by its prefix). Pure, so poses can be tested without a window.
 */
#ifndef GAME_RENDER_POSE_H_
#define GAME_RENDER_POSE_H_

#include "world.h"

typedef enum {
  STRIP_FAB_IDLE,
  STRIP_FAB_WALK,
  STRIP_FAB_RUN,
  STRIP_FAB_JUMP,
  STRIP_FAB_ATTACK,
  STRIP_FAB_HURT,
  STRIP_FAB_GUNWALK,
  STRIP_FAB_GUNAIM,
  STRIP_FAB_GUNSHOOT,
  STRIP_FAB_POTION,
  STRIP_FAB_DRINK,
  STRIP_FAB_SPELL,
  STRIP_FAB_CAST,
  STRIP_ZOMBIE,
  STRIP_ZOMBIE_ATTACK,
  STRIP_BUG_BEETLE,
  STRIP_BUG_FLY,
  STRIP_BUG_BEE,
  STRIP_BUG_DARK,
  STRIP_BUG_SMALL,
  STRIP_FX_MUZZLE,
  STRIP_FX_BULLET,
  STRIP_FX_RAY,
  STRIP_FX_BUG_SPLAT,
  STRIP_FX_BUG_BOOM,
  STRIP_FX_BUG_BLUE,
  STRIP_FX_ZOMBIE_GREEN,
  STRIP_FX_ZOMBIE_BOOM,
  STRIP_FX_SMOKE,
  STRIP_FX_REMAINS,
  STRIP_PISTOL,
  STRIP_BOTTLE,
  STRIP_COIN,
  STRIP_MUSHROOM,
  STRIP_TILE_SOLID,
  STRIP_TILE_Q, /* yellow; the next three are blue, grey and orange, in this order */
  STRIP_TILE_Q_BLUE,
  STRIP_TILE_Q_GREY,
  STRIP_TILE_Q_ORANGE,
  STRIP_TILE_BRICK,
  STRIP_TILE_GOLD,
  STRIP_TILE_STONE,
  STRIP_GROUND_GRASS,
  STRIP_GROUND_DIRT,
  STRIP_GROUND_ROCK,
  STRIP_PIPE_CAP,
  STRIP_PIPE_BODY,
  STRIP_BUBBLE,
  STRIP_READER_READ,
  STRIP_READER_LAUGH,
  STRIP_READER_POP,
  STRIP_READER_FLASH,
  STRIP_READER_BOOM,
  STRIP_READER_REMAINS,
  STRIP_MONK_CROSS,
  STRIP_MONK_BOOM,
  STRIP_SKELETON_WALK,
  STRIP_SKELETON_VANISH,
  STRIP_BUBBLE_MONK,
  STRIP_BUBBLE_SKELETON,
  STRIP_CHEF_HAPPY,
  STRIP_CHEF_ATTACK,
  STRIP_CHEF_BOOM,
  STRIP_BUBBLE_CHEF,
  STRIP_GORILLA_IDLE,
  STRIP_GORILLA_ROAR,
  STRIP_GORILLA_BEAT,
  STRIP_GORILLA_THROW,
  STRIP_BANANA,
  STRIP_DEMON_STAND,
  STRIP_DEMON_CAST,
  STRIP_DEMON_DEATH,
  STRIP_FIREBALL,
  STRIP_VEGETABLE,
  STRIP_SKEWER,
  STRIP_HELI_REST,
  STRIP_FAB_HELI,
  STRIP_HELI_BOOM,
  STRIP_DRUMS_REST,
  STRIP_DRUMS_PLAY,
  STRIP_FAB_BUB_IDLE, /* Fabio with the soap bubble shotgun */
  STRIP_FAB_BUB_WALK,
  STRIP_FAB_BUB_RUN,
  STRIP_FAB_BUB_JUMP,
  STRIP_FAB_BUB_AIM,
  STRIP_FAB_BUB_SHOOT,
  STRIP_BUBBLE_GUN, /* the gun on its own: the pickup */
  STRIP_SLOP_IDLE, /* the AI slop monster */
  STRIP_SLOP_WINDUP,
  STRIP_SLOP_THROW,
  STRIP_SLOP_DEATH,
  STRIP_SLOP_BALL,
  STRIP_SLOP_SPLAT,
  STRIP_SLOP_PIECE,
  STRIP_FAB_JEDI_IDLE, /* Fabio turned Jedi knight, with the green lightsaber */
  STRIP_FAB_JEDI_WALK,
  STRIP_FAB_JEDI_RUN,
  STRIP_FAB_JEDI_JUMP,
  STRIP_FAB_JEDI_SWING,
  STRIP_FAB_JEDI_HURT,
  STRIP_SABER,        /* the lightsaber on its own: the pickup */
  STRIP_FORCE_BURST,  /* the green burst in flight */
  STRIP_FORCE_HIT,    /* and bursting on an enemy */
  STRIP_OLD_KNIGHT,   /* the old knight's face: mouth shut, mouth open */
  STRIP_SOAP_BUBBLE, /* one round soap bubble, drawn at any size */
  STRIP_COUNT
} strip_id_t;

typedef struct {
  const char* prefix;
  int frames; /* 0: a single image named `prefix` */
} strip_spec_t;

extern const strip_spec_t kStripSpecs[STRIP_COUNT];

typedef struct {
  strip_id_t strip;
  int frame;
} pose_t;

pose_t pose_player(const player_t* player);
/* The hooded monk prays while Fabio is far, and shows the cross, more and more
 * boldly, as he comes close. */
pose_t pose_monk(const enemy_t* monk, const player_t* player);
pose_t pose_zombie(const enemy_t* zombie, const player_t* player);
pose_t pose_bug(const enemy_t* bug);
pose_t pose_effect(fx_kind_t kind);
pose_t pose_book_reader(const enemy_t* book_reader);
/* The book reader bursting: he swells, flashes, booms and leaves smoking
 * remains. `age` is seconds since the shot. */
pose_t pose_book_reader_boom(float age);
/* The monk goes up in an explosion that thins to smoke and then a little
 * debris. `age` is seconds since the hit. */
pose_t pose_monk_boom(float age);
/* The skeleton's walk cycle; he stands on the first frame when held up. */
pose_t pose_skeleton(const enemy_t* skeleton);
/* The skeleton dissolves into dust fast: a frame every 45 milliseconds. */
pose_t pose_skeleton_vanish(float age);
/* The chef smiles and points while Fabio is far (his smile frames, there and
 * back), and swings the knife once Fabio is close. */
pose_t pose_chef(const enemy_t* chef);
/* The chef reels from the hit, then explodes into pieces and smoke. `age` is
 * seconds since the hit. */
pose_t pose_chef_death(float age);
/* The gorilla standing, roaring, beating his chest (two frames, quickly back
 * and forth) and throwing, by gorilla_act. */
pose_t pose_gorilla(const enemy_t* gorilla);
/* The demon shifting where he stands, then casting through his four frames. */
pose_t pose_demon(const enemy_t* demon);
/* The demon clutching himself and dissolving: eight frames over DEMON_DEATH_SECONDS. */
pose_t pose_demon_death(float age);
/* The helicopter blowing up: seven frames over HELICOPTER_BOOM_SECONDS. */
pose_t pose_helicopter_boom(float age);
/* The drum kit on its own, or with Fabio flat out at it: DRUM_FRAMES frames, flashing by. */
pose_t pose_drums(const drum_kit_t* kit, float time);
/* The green burst flying, and the sparks it leaves on an enemy (`age` of `duration`). */
pose_t pose_force_burst(float age);
pose_t pose_force_hit(float age, float duration);
/* The old knight at the top left: his mouth moves while he is speaking and is shut as he fades. */
pose_t pose_old_knight(float left);
/* The slop monster dripping, winding up a ball over his head, and throwing it. */
pose_t pose_slop(const enemy_t* slop);
/* The slop effects: the monster's death (FX_SLOP_DEATH) and a ball landing, `age` of `duration`. */
pose_t pose_slop_effect(fx_kind_t kind, float age, float duration);

/* The "?" blocks all turn round together, then rest facing out, by world time. */
#define QBLOCK_SPIN_SECONDS 1.0f
#define QBLOCK_REST_SECONDS 0.8f
#define QBLOCK_COLOURS 4
/* Which of the four colours the block at (col, row) is, always the same one. */
int question_block_colour(int col, int row);
pose_t pose_question_block(float time, int col, int row);

#endif  // GAME_RENDER_POSE_H_
