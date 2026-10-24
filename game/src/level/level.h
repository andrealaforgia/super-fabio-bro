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
#define MAX_LEVEL_ITEMS 512
#define MAX_LEVEL_DECOR 256

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
  ITEM_MUSHROOM
} item_kind_t;

typedef enum { DECOR_PINE, DECOR_OAK, DECOR_BUSH } decor_kind_t;

typedef struct {
  item_kind_t kind;
  float x, y; /* centre */
} placement_t;

typedef struct {
  decor_kind_t kind;
  float x, y; /* middle of the base */
} decor_t;

typedef struct {
  uint8_t tiles[LEVEL_ROWS][LEVEL_COLS];
  uint8_t content[LEVEL_ROWS][LEVEL_COLS]; /* item_kind_t inside a ? block */
  placement_t items[MAX_LEVEL_ITEMS];
  int item_count;
  decor_t decor[MAX_LEVEL_DECOR];
  int decor_count;
  float spawn_x, spawn_y; /* middle of the feet */
  float castle_x;         /* middle of the castle */
  float goal_x;           /* reaching it wins */
} level_t;

bool tile_is_solid(tile_t tile);
/* Out of range: solid walls left and right, open sky above, a pit below. */
tile_t level_tile(const level_t* level, int col, int row);
bool level_solid_at(const level_t* level, float x, float y);
float level_width(void);
float level_height(void);

void level_generate(level_t* level, uint32_t seed);

#endif  // GAME_LEVEL_LEVEL_H_
