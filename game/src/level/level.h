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
#define LEVEL_COLS 1000
#define GROUND_ROW 13
#define MAX_DRUM_KITS 8
#define MAX_LEVEL_ITEMS 1536
#define MAX_LEVEL_DECOR 384
#define MAX_LEVEL_PLATFORMS 320
#define MAX_BUILDINGS 64
#define MAX_VASES 128
#define MAX_SPOUTS 8
#define MAX_WARPS 12
#define CAVE_COLS 48
#define DESERT_COL 150 /* where the desert begins, and */
#define SNOW_COL 310   /* the snow, in the main level */
#define CITY_COL 440   /* and the cities, London first and Sulmona last, just before the castle */
#define CITY_COUNT 6
#define EYE_PODS 12          /* the capsules of the London Eye */
#define EYE_RADIUS 150.0f    /* from the hub to the rim */
#define EYE_PERIOD 42.0f     /* seconds for a full turn */
#define POD_DROP 16.0f       /* a capsule's floor hangs this far below its place on the rim */
#define POD_W 64.0f
#define BALCONY_STEP 96.0f /* the height of a floor: every balcony and roof is a multiple of it above the street */

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
typedef enum { BIOME_FOREST, BIOME_DESERT, BIOME_SNOW, BIOME_CAVE, BIOME_CITY, BIOME_COUNT } biome_t;

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
  DECOR_CAR,        /* the city's parked cars: four variants */
  DECOR_OVID,       /* the statue of Ovid */
  DECOR_FOUNTAIN,   /* a fountain in the piazza: two frames */
  DECOR_WELCOME,    /* the sign "Welcome to Sulmona!", drawn in code from here on */
  DECOR_SIGNPOST,   /* Pescara one way, L'Aquila the other */
  DECOR_BIG_BEN,    /* the landmarks of the cities, all drawn in code */
  DECOR_BUS,        /* a London double-decker */
  DECOR_ABBEY,      /* the gateway of Reading Abbey */
  DECOR_BISCUIT_TIN,
  DECOR_BULL,       /* the Bull of Birmingham's Bullring */
  DECOR_ROTUNDA,
  DECOR_DUOMO,      /* Milan cathedral */
  DECOR_TRAM,
  DECOR_COLOSSO,    /* the giant bronze of Barletta */
  DECOR_CASTELLO,
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
typedef enum {
  PLATFORM_SLIDER,
  PLATFORM_LIFT,
  PLATFORM_STATIC, /* a city balcony, roof, aqueduct deck or car roof: it stays put, and the city draws it */
  PLATFORM_CAPSULE /* a London Eye capsule: (x, y) is the hub, `range` the radius; it stays level as it goes round */
} platform_kind_t;

typedef struct {
  platform_kind_t kind;
  float x, y;    /* middle of the plank's top, in the middle of its travel */
  float w;       /* the plank's width */
  float range;   /* how far it swings either way from the middle */
  float period;  /* seconds for a full swing there and back */
  float phase;   /* radians into the swing at time zero */
} platform_def_t;

/* The city's buildings are drawn in code, in front of the sky and behind Fabio: only their
 * balconies and roofs (static planks) can be stood on. */
typedef enum {
  BUILDING_APARTMENT,
  BUILDING_HOTEL,
  BUILDING_THAI,
  BUILDING_SHOP,
  BUILDING_GELATERIA,
  BUILDING_BARBIERE,
  BUILDING_RISTORANTE,
  BUILDING_SUPERMERCATO,
  BUILDING_OFFICINA,
  BUILDING_FIORERIA,
  BUILDING_CALZOLAIO,
  BUILDING_KIND_COUNT
} building_kind_t;
typedef struct {
  building_kind_t kind;
  int variant;
  float x;     /* left edge */
  float w;
  int floors;  /* the roof is `floors` * BALCONY_STEP up */
  bool english; /* the shop signs are in English: the cities of the United Kingdom */
} building_t;
/* What the sign over a building's door says. */
const char* building_name(building_kind_t kind, bool english);

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
  building_t buildings[MAX_BUILDINGS];
  int building_count;
  int vase_count;                 /* terracotta vases standing on the balconies */
  float vase_x[MAX_VASES], vase_y[MAX_VASES]; /* the middle of the base */
  int vase_plank[MAX_VASES];      /* the balcony each stands on */
  bool has_bull;                  /* the Bullring's bull statue, which comes alive */
  float bull_x;
  int bull_plank;                 /* the ledge on its back, which is gone while it runs */
  bool has_eye;                   /* the London Eye */
  float eye_x, eye_y;             /* its hub */
  int eye_first_pod;              /* the platform index of its first capsule */
  float terrace_x0, terrace_x1, terrace_y; /* the viewing deck beside it */
  bool has_aqueduct;              /* the medieval aqueduct, with its tap */
  float aqueduct_x0, aqueduct_x1; /* where it begins and ends */
  float aqueduct_deck;            /* the walkway along the top: its y */
  float tap_x;                    /* the water tap on the street: up turns it on */
  int spout_count;
  float spout_x[MAX_SPOUTS];      /* the middle of each water curtain */
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
extern const char* const kCityNames[CITY_COUNT];
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
