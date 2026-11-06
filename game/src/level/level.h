/**
 * @file level.h
 * @brief The tile map of Super Fabio Bros and its deterministic generator.
 *
 * World units are pixels, 32 to a tile, y growing down. The map is
 * LEVEL_ROWS tall, which is exactly one screen, so the camera only scrolls
 * sideways.
 */
#ifndef GAME_LEVEL_LEVEL_H_
#define GAME_LEVEL_LEVEL_H_

#include <stdbool.h>
#include <stdint.h>

#define TILE 32
#define LEVEL_ROWS 17
#define LEVEL_COLS 520
#define GROUND_ROW 13
#define MAX_DRUM_KITS 8
#define MAX_LEVEL_ITEMS 512
#define MAX_LEVEL_DECOR 256
#define MAX_LEVEL_PLATFORMS 64
#define MAX_WARPS 12
#define CAVE_COLS 48
#define DESERT_COL 150 /* where the desert begins, and */
#define SNOW_COL 310   /* the snow, in the main level */

typedef enum {
  TILE_EMPTY = 0,
  TILE_GRASS,
  TILE_ROCK,
  TILE_BRICK,
  TILE_QBLOCK,
  TILE_USED,
  TILE_GOLD,
  TILE_STONE,
  TILE_PIPE_TOP_L,
  TILE_PIPE_TOP_R,
  TILE_PIPE_BODY_L,
  TILE_PIPE_BODY_R
} tile_t;

typedef enum {
  ITEM_NONE = 0,
  ITEM_COIN,
  ITEM_POTION,
  ITEM_PISTOL,
  ITEM_MUSHROOM,
  ITEM_BUBBLE_GUN,
  ITEM_LIGHTSABER,
  ITEM_SUSHI,  /* a sushi dumpling: Andrea turns up */
  ITEM_DRAGON, /* a Chinese dragon: Alessandro */
  ITEM_GLOVES  /* boxing gloves: Giulia */
} item_kind_t;

/* The scenery zones: the level runs from forest through desert into snow before the
 * castle, and a secret cave sits under the warp pipes. */
typedef enum { BIOME_FOREST, BIOME_DESERT, BIOME_SNOW, BIOME_CAVE, BIOME_COUNT } biome_t;

/* The first three are drawn procedurally (look.trees); the rest are cooked sprites, one strip
 * per kind with `variant` picking the frame. */
typedef enum {
  DECOR_PINE,
  DECOR_OAK,
  DECOR_BUSH,
  DECOR_SNOW_PINE,
  DECOR_SNOWMAN,
  DECOR_ICE,
  DECOR_FROST_BUSH,
  DECOR_CACTUS,
  DECOR_DEAD_TREE,
  DECOR_SKULL,
  DECOR_BOULDER,
  DECOR_CRYSTAL,
  DECOR_MUSHROOM,
  DECOR_STALAGMITE,
  DECOR_STALACTITE, /* hangs from the ceiling: `y` is where it attaches, not a base */
  DECOR_KIND_COUNT
} decor_kind_t;
#define DECOR_PROCEDURAL 3

typedef struct {
  item_kind_t kind;
  float x, y; /* centre */
} placement_t;

typedef struct {
  decor_kind_t kind;
  int variant; /* which frame of its strip, for the cooked kinds */
  float x, y;  /* middle of the base */
} decor_t;

/* Planks that carry Fabio: a slider swings left and right across a pit, a lift
 * rises and falls beside a ledge. Each is a pure function of the time, so the
 * level only stores where it swings about and how far. A plank is solid from
 * above only. */
typedef enum { PLATFORM_SLIDER, PLATFORM_LIFT } platform_kind_t;

typedef struct {
  platform_kind_t kind;
  float x, y;    /* middle of the plank's top, in the middle of its travel */
  float w;       /* the plank's width */
  float range;   /* how far it swings either way from the middle */
  float period;  /* seconds for a full swing there and back */
  float phase;   /* radians into the swing at time zero */
} platform_def_t;

typedef struct {
  uint8_t tiles[LEVEL_ROWS][LEVEL_COLS];
  uint8_t content[LEVEL_ROWS][LEVEL_COLS]; /* item_kind_t inside a ? block */
  placement_t items[MAX_LEVEL_ITEMS];
  int item_count;
  decor_t decor[MAX_LEVEL_DECOR];
  int decor_count;
  platform_def_t platforms[MAX_LEVEL_PLATFORMS];
  int platform_count;
  bool has_helicopter;    /* the one helicopter resting on the grass */
  float helicopter_x, helicopter_y; /* middle, and the skids */
  int drum_count;         /* Fabio's drum kits, one now and then all along */
  float drums_x[MAX_DRUM_KITS]; /* middles */
  float drums_y;                /* the base of them all */
  int cols;               /* columns in use: LEVEL_COLS, or CAVE_COLS in a cave */
  bool cave;              /* a secret cave under a warp pipe, not the main level */
  int warp_count;         /* warp pipes: Fabio presses down on one to go through */
  int warp_col[MAX_WARPS]; /* the left column of each */
  int warp_row[MAX_WARPS]; /* and its top row: he stands on the cap */
  int guardian;           /* in a cave: 0 none, 1 a gorilla, 2 the slop monster */
  float guardian_x;
  float spawn_x, spawn_y; /* middle of the feet */
  float castle_x;         /* middle of the castle */
  float goal_x;           /* reaching it wins */
} level_t;

bool tile_is_solid(tile_t tile);
/* Out of range: solid walls left and right, open sky above, a pit below. */
tile_t level_tile(const level_t* level, int col, int row);
bool level_solid_at(const level_t* level, float x, float y);
float level_width(void);
/* The width of this level in pixels: the main level's, or the cave's. */
float level_pixel_width(const level_t* level);
/* Which zone column `col` is in; the whole of a cave is BIOME_CAVE. */
biome_t level_biome_at(const level_t* level, int col);
float level_height(void);

/* Where the middle of the top of a plank is `time` seconds into the level. */
void platform_position(const platform_def_t* plank, float time, float* x, float* y);

void level_generate(level_t* level, uint32_t seed);
/* The secret cave under the warp pipe `index`: a treasure room with a guardian and
 * a pipe at each end that leads back up. Always the same for the same seed. */
void level_generate_cave(level_t* level, uint32_t seed, int index);

#endif  // GAME_LEVEL_LEVEL_H_
