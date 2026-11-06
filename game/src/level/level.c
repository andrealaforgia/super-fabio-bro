#include "level.h"

#include <math.h>
#include <string.h>

#include "rng.h"

bool tile_is_solid(tile_t tile) { return tile != TILE_EMPTY; }

tile_t level_tile(const level_t* level, int col, int row) {
  if (col < 0 || col >= LEVEL_COLS) return TILE_ROCK;
  if (row < 0 || row >= LEVEL_ROWS) return TILE_EMPTY;
  return (tile_t)level->tiles[row][col];
}

bool level_solid_at(const level_t* level, float x, float y) {
  int col = (int)(x / TILE) - (x < 0 ? 1 : 0);
  int row = (int)(y / TILE) - (y < 0 ? 1 : 0);
  return tile_is_solid(level_tile(level, col, row));
}

float level_width(void) { return (float)(LEVEL_COLS * TILE); }
float level_pixel_width(const level_t* level) {
  return (float)((level->cols > 0 ? level->cols : LEVEL_COLS) * TILE);
}

biome_t level_biome_at(const level_t* level, int col) {
  if (level->cave) return BIOME_CAVE;
  return col >= SNOW_COL ? BIOME_SNOW : (col >= DESERT_COL ? BIOME_DESERT : BIOME_FOREST);
}
float level_height(void) { return (float)(LEVEL_ROWS * TILE); }

void platform_position(const platform_def_t* plank, float time, float* x, float* y) {
  float swing = plank->range * sinf(6.2831853f * time / plank->period + plank->phase);
  *x = plank->kind == PLATFORM_SLIDER ? plank->x + swing : plank->x;
  *y = plank->kind == PLATFORM_LIFT ? plank->y + swing : plank->y;
}

typedef struct {
  level_t* level;
  rng_t rng;
  int x;               /* next free column */
  int since_hazard;    /* flat columns since the last pit, pipe or stairs */
  int since_potion;    /* columns since a potion was made available */
  int since_pistol;
  int since_bubble_gun;
  int since_saber;
  int since_ally;
  int last_decor;
  int counted_x;       /* columns already added to the since_* counters */
  int last_drums;      /* the column of the latest drum kit */
  int last_warp;       /* the column of the latest warp pipe */
} generator_t;

static void put(generator_t* g, int col, int row, tile_t tile) {
  if (col >= 0 && col < LEVEL_COLS && row >= 0 && row < LEVEL_ROWS)
    g->level->tiles[row][col] = (uint8_t)tile;
}

static void add_item(generator_t* g, item_kind_t kind, int col, int row) {
  level_t* level = g->level;
  if (level->item_count >= MAX_LEVEL_ITEMS) return;
  placement_t* item = &level->items[level->item_count++];
  item->kind = kind;
  item->x = (float)(col * TILE + TILE / 2);
  item->y = (float)(row * TILE + TILE / 2);
}

static void add_decor(generator_t* g, decor_kind_t kind, int col) {
  level_t* level = g->level;
  if (level->decor_count >= MAX_LEVEL_DECOR) return;
  decor_t* decor = &level->decor[level->decor_count++];
  decor->kind = kind;
  decor->variant = 0;
  decor->x = (float)(col * TILE + TILE / 2);
  decor->y = (float)(GROUND_ROW * TILE);
  g->last_decor = col;
}

static void add_platform(generator_t* g, platform_kind_t kind, float x, float y, float w,
                         float range, float period, float phase) {
  level_t* level = g->level;
  if (level->platform_count >= MAX_LEVEL_PLATFORMS) return;
  level->platforms[level->platform_count++] =
      (platform_def_t){kind, x, y, w, range, period, phase};
}

static void ground_column(generator_t* g, int col) {
  put(g, col, GROUND_ROW, TILE_GRASS);
  for (int row = GROUND_ROW + 1; row < LEVEL_ROWS; row++)
    put(g, col, row, TILE_ROCK);
}

static void flat(generator_t* g, int length, bool scenery) {
  for (int i = 0; i < length && g->x < LEVEL_COLS; i++) {
    ground_column(g, g->x);
    if (scenery && g->x - g->last_decor >= 4 && rng_chance(&g->rng, 0.22f)) {
      int roll = rng_range(&g->rng, 0, 9);
      biome_t biome = level_biome_at(g->level, g->x);
      if (biome == BIOME_DESERT) {
        add_decor(g, roll < 5 ? DECOR_CACTUS : (roll < 7 ? DECOR_DEAD_TREE : (roll < 9 ? DECOR_BOULDER : DECOR_SKULL)), g->x);
        g->level->decor[g->level->decor_count - 1].variant = rng_range(&g->rng, 0, 2);
      } else if (biome == BIOME_SNOW) {
        add_decor(g, roll < 4 ? DECOR_SNOW_PINE : (roll < 6 ? DECOR_ICE : (roll < 8 ? DECOR_FROST_BUSH : DECOR_SNOWMAN)), g->x);
        g->level->decor[g->level->decor_count - 1].variant = rng_range(&g->rng, 0, 2);
      } else {
        add_decor(g, roll < 5 ? DECOR_PINE : (roll < 8 ? DECOR_OAK : DECOR_BUSH), g->x);
      }
    }
    if (scenery && rng_chance(&g->rng, 0.05f)) {
      int arc = rng_range(&g->rng, 3, 5);
      for (int c = 0; c < arc; c++)
        add_item(g, ITEM_COIN, g->x + c, GROUND_ROW - 2 - (c == 0 || c == arc - 1 ? 0 : 1));
    }
    g->x++;
  }
  g->since_hazard += length;
}

/* The one helicopter rests in the middle of the first long stretch of open grass
 * after HELICOPTER_FROM_COL (about a quarter of the way in). */
#define HELICOPTER_FROM_COL 120
#define HELICOPTER_MIN_FLAT 8

static void rest_helicopter(generator_t* g, int length) {
  level_t* level = g->level;
  if (level->has_helicopter || g->x < HELICOPTER_FROM_COL || length < HELICOPTER_MIN_FLAT) return;
  level->has_helicopter = true;
  level->helicopter_x = (float)((g->x + length / 2) * TILE + TILE / 2);
  level->helicopter_y = (float)(GROUND_ROW * TILE);
}

/* Drum kits: one at the start, then one on the open grass every DRUMS_EVERY
 * columns or so, never on the helicopter's stretch. */
#define DRUMS_EVERY 70
#define WARP_OVERDUE 90 /* columns without a warp pipe, after which the next hazard is one */
#define DRUMS_OVERDUE 100 /* past this, the next stretch is made open grass for one */

static void stand_drums_at(generator_t* g, int col) {
  level_t* level = g->level;
  if (level->drum_count >= MAX_DRUM_KITS) return;
  level->drums_x[level->drum_count++] = (float)(col * TILE + TILE / 2);
  level->drums_y = (float)(GROUND_ROW * TILE);
  g->last_drums = col;
}

static void stand_drums(generator_t* g, int length) {
  level_t* level = g->level;
  if (g->x < g->last_drums + DRUMS_EVERY || length < HELICOPTER_MIN_FLAT) return;
  float from = (float)(g->x * TILE), to = (float)((g->x + length) * TILE);
  if (level->has_helicopter && level->helicopter_x >= from && level->helicopter_x <= to) return;
  stand_drums_at(g, g->x + length / 2);
}

static item_kind_t roll_content(generator_t* g) {
  if (g->since_potion > 45 || rng_chance(&g->rng, 0.1f)) {
    g->since_potion = 0;
    return ITEM_POTION;
  }
  if (g->since_pistol > 110 || rng_chance(&g->rng, 0.04f)) {
    g->since_pistol = 0;
    return ITEM_PISTOL;
  }
  if (g->since_bubble_gun > 160 || rng_chance(&g->rng, 0.03f)) {
    g->since_bubble_gun = 0;
    return ITEM_BUBBLE_GUN;
  }
  if (g->since_saber > 150 || rng_chance(&g->rng, 0.03f)) {
    g->since_saber = 0;
    return ITEM_LIGHTSABER;
  }
  if (g->since_ally > 130 || rng_chance(&g->rng, 0.04f)) {
    g->since_ally = 0;
    int pick = rng_range(&g->rng, 0, 2);
    return pick == 0 ? ITEM_SUSHI : (pick == 1 ? ITEM_DRAGON : ITEM_GLOVES);
  }
  if (rng_chance(&g->rng, 0.2f)) return ITEM_MUSHROOM;
  return ITEM_COIN;
}

static void question_block(generator_t* g, int col, int row, item_kind_t what) {
  put(g, col, row, TILE_QBLOCK);
  g->level->content[row][col] = (uint8_t)what;
}

static void platform(generator_t* g, int length) {
  flat(g, 3, true);
  int left = g->x;
  int total = length + 4;
  for (int i = 0; i < total; i++) ground_column(g, g->x + i);
  int row = GROUND_ROW - 4;
  bool has_question = false;
  for (int i = 0; i < length; i++) {
    int col = left + 2 + i;
    bool ask = rng_chance(&g->rng, 0.3f) || (i == length / 2 && !has_question);
    if (ask) {
      question_block(g, col, row, roll_content(g));
      has_question = true;
    } else {
      put(g, col, row, TILE_BRICK);
    }
  }
  if (length >= 5 && rng_chance(&g->rng, 0.6f)) {
    int upper = length - 3;
    int start = left + 2 + (length - upper) / 2;
    for (int i = 0; i < upper; i++) put(g, start + i, GROUND_ROW - 8, TILE_GOLD);
    int pick = rng_range(&g->rng, 0, upper - 1);
    for (int i = 0; i < upper; i++)
      add_item(g, i == pick && g->since_potion > 20 ? ITEM_POTION : ITEM_COIN,
               start + i, GROUND_ROW - 9);
    if (g->since_potion > 20) g->since_potion = 0;
  } else {
    for (int i = 0; i < length; i++)
      if (rng_chance(&g->rng, 0.5f))
        add_item(g, ITEM_COIN, left + 2 + i, GROUND_ROW - 6);
  }
  g->x += total;
  g->since_hazard += total;
}

static void pipe(generator_t* g) {
  flat(g, 2, true);
  int height = rng_range(&g->rng, 2, 3);
  for (int dx = 0; dx < 2; dx++) {
    ground_column(g, g->x + dx);
    for (int h = 1; h <= height; h++) {
      bool top = h == height;
      put(g, g->x + dx, GROUND_ROW - h,
          top ? (dx ? TILE_PIPE_TOP_R : TILE_PIPE_TOP_L)
              : (dx ? TILE_PIPE_BODY_R : TILE_PIPE_BODY_L));
    }
  }
  if (rng_chance(&g->rng, 0.5f)) {
    add_item(g, ITEM_COIN, g->x, GROUND_ROW - height - 1);
    add_item(g, ITEM_COIN, g->x + 1, GROUND_ROW - height - 1);
  }
  /* Every pipe is a warp pipe: down it, and under it, there is a cave. */
  if (g->level->warp_count < MAX_WARPS) {
    g->level->warp_col[g->level->warp_count] = g->x;
    g->level->warp_row[g->level->warp_count] = GROUND_ROW - height;
    g->level->warp_count++;
  }
  g->last_warp = g->x;
  g->x += 2;
  g->since_hazard = 0;
}

static void pit(generator_t* g, int width) {
  bool stones = width >= 5;
  for (int i = 0; i < width; i++) {
    if (stones && (i == 1 || i == width - 2)) {
      put(g, g->x + i, GROUND_ROW - 3, TILE_GOLD);
      add_item(g, ITEM_COIN, g->x + i, GROUND_ROW - 4);
    }
  }
  if (width >= 4 && rng_chance(&g->rng, 0.85f)) {
    /* A plank swings from one side of the pit to the other, level with the ground. */
    float plank = 3.0f * TILE;
    float range = ((float)width * TILE - plank) * 0.5f;
    add_platform(g, PLATFORM_SLIDER, (float)(g->x * TILE) + (float)width * TILE * 0.5f,
                 (float)(GROUND_ROW * TILE), plank, range, 3.0f + range / 48.0f,
                 rng_chance(&g->rng, 0.5f) ? 0.0f : 3.1415927f);
  }
  g->x += width;
  g->since_hazard = 0;
}

/* A gold ledge high above the ground, with a lift beside it that carries Fabio up. */
static void lift_ledge(generator_t* g) {
  flat(g, 3, true);
  int left = g->x;
  int length = rng_range(&g->rng, 4, 6);
  for (int i = 0; i < 2 + length; i++) ground_column(g, left + i);
  int ledge_row = GROUND_ROW - 8;
  for (int i = 0; i < length; i++) {
    put(g, left + 2 + i, ledge_row, TILE_GOLD);
    add_item(g, ITEM_COIN, left + 2 + i, ledge_row - 1);
  }
  float bottom = (float)(GROUND_ROW * TILE) - 8.0f;
  float top = (float)(ledge_row * TILE);
  add_platform(g, PLATFORM_LIFT, (float)((left + 1) * TILE) - 12.0f, (bottom + top) * 0.5f, 2.0f * TILE,
               (bottom - top) * 0.5f, 8.0f, 1.5707963f); /* it starts at the bottom */
  g->x += 2 + length;
  g->since_hazard += 2 + length;
}

static void stairs(generator_t* g, int steps, bool descend) {
  for (int i = 0; i < steps; i++) {
    int height = descend ? steps - i : i + 1;
    ground_column(g, g->x);
    for (int h = 1; h <= height; h++) put(g, g->x, GROUND_ROW - h, TILE_STONE);
    g->x++;
  }
  g->since_hazard = 0;
}

static void castle(generator_t* g) {
  level_t* level = g->level;
  int steps = 5;
  flat(g, 6, false);
  stairs(g, steps, false);
  for (int i = 0; i < 3; i++) {
    ground_column(g, g->x);
    for (int h = 1; h <= steps; h++) put(g, g->x, GROUND_ROW - h, TILE_STONE);
    g->x++;
  }
  stairs(g, steps, true);
  flat(g, 4, false);
  level->castle_x = (float)(g->x * TILE + 3 * TILE);
  level->goal_x = level->castle_x;
  flat(g, LEVEL_COLS - g->x, false);
}

void level_generate(level_t* level, uint32_t seed) {
  memset(level, 0, sizeof(*level));
  generator_t gen;
  memset(&gen, 0, sizeof(gen));
  generator_t* g = &gen;
  g->level = level;
  rng_seed(&g->rng, seed);
  g->last_decor = -10;
  g->since_hazard = 100;

  level->cols = LEVEL_COLS;
  level->spawn_x = 3.0f * TILE;
  level->spawn_y = (float)(GROUND_ROW * TILE);

  /* The opening: room to move, then a row of bricks with the pistol and a
   * potion in the first question blocks. */
  /* No drum kit near the start: the first is due 70 columns after column 25, so from about column 95 (and by column 125 at the latest, when a stretch of grass is made for it). */
  g->last_drums = 25;
  flat(g, 9, true);
  int left = g->x;
  for (int i = 0; i < 9; i++) ground_column(g, left + i);
  int row = GROUND_ROW - 4;
  question_block(g, left, row, ITEM_GLOVES);
  question_block(g, left + 1, row, ITEM_SUSHI);
  question_block(g, left + 2, row, ITEM_PISTOL);
  question_block(g, left + 3, row, ITEM_LIGHTSABER);
  question_block(g, left + 4, row, ITEM_POTION);
  question_block(g, left + 5, row, ITEM_DRAGON);
  question_block(g, left + 6, row, ITEM_BUBBLE_GUN);
  add_item(g, ITEM_COIN, left + 7, GROUND_ROW - 2);
  g->x += 9;
  g->since_potion = 0;
  g->since_pistol = 0;
  g->since_bubble_gun = 0;
  g->since_saber = 0;
  g->since_ally = 0;
  pipe(g); /* a warp pipe to try, just past the opening */

  g->counted_x = g->x;
  while (g->x < LEVEL_COLS - 52) {
    g->since_potion += g->x - g->counted_x;
    g->since_pistol += g->x - g->counted_x;
    g->since_bubble_gun += g->x - g->counted_x;
    g->since_saber += g->x - g->counted_x;
    g->since_ally += g->x - g->counted_x;
    g->counted_x = g->x;
    if (g->since_hazard < 6) {
      flat(g, rng_range(&g->rng, 3, 6), true);
      continue;
    }
    int roll = rng_range(&g->rng, 0, 99);
    bool potion_due = g->since_potion > 52;
    if (g->x >= g->last_drums + DRUMS_OVERDUE) roll = 0; /* open grass for the drums */
    else if (g->x >= g->last_warp + WARP_OVERDUE && !potion_due) roll = 80; /* a pipe, when it has been too long */
    if (potion_due && roll >= 22) roll = 30; /* a platform, and a potion on it */
    if (roll < 22) {
      int length = rng_range(&g->rng, 6, 14);
      rest_helicopter(g, length);
      stand_drums(g, length);
      flat(g, length, true);
    } else if (roll < 50) {
      platform(g, rng_range(&g->rng, 3, 8));
    } else if (roll < 66) {
      pit(g, rng_range(&g->rng, 2, 6));
      flat(g, 6, false);
    } else if (roll < 74) {
      lift_ledge(g);
    } else if (roll < 84) {
      pipe(g);
    } else {
      int steps = rng_range(&g->rng, 2, 4);
      stairs(g, steps, false);
      flat(g, 1, false);
      stairs(g, steps, true);
    }
  }
  castle(g);
}

/* A pipe in the cave: two wide, `height` tall, standing on the cave floor. */
static void cave_pipe(generator_t* g, int col, int height) {
  for (int dx = 0; dx < 2; dx++)
    for (int h = 1; h <= height; h++) {
      bool top = h == height;
      put(g, col + dx, GROUND_ROW - h,
          top ? (dx ? TILE_PIPE_TOP_R : TILE_PIPE_TOP_L) : (dx ? TILE_PIPE_BODY_R : TILE_PIPE_BODY_L));
    }
  g->level->warp_col[g->level->warp_count] = col;
  g->level->warp_row[g->level->warp_count] = GROUND_ROW - height;
  g->level->warp_count++;
}

static void cave_decor(generator_t* g, decor_kind_t kind, int col, int variant, float y) {
  add_decor(g, kind, col);
  decor_t* d = &g->level->decor[g->level->decor_count - 1];
  d->variant = variant;
  d->y = y;
}

void level_generate_cave(level_t* level, uint32_t seed, int index) {
  memset(level, 0, sizeof(*level));
  generator_t gen;
  memset(&gen, 0, sizeof(gen));
  generator_t* g = &gen;
  g->level = level;
  rng_seed(&g->rng, seed * 2654435761u + (uint32_t)index * 40503u + 17u);
  g->last_decor = -10;
  level->cave = true;
  level->cols = CAVE_COLS;
  /* Nothing ever wins in a cave: the goal is out of reach. */
  level->castle_x = level->goal_x = 1.0e7f;

  /* Solid rock beyond the cave, a floor, a three-row ceiling and a wall at each end. */
  for (int col = 0; col < LEVEL_COLS; col++)
    for (int row = 0; row < LEVEL_ROWS; row++) put(g, col, row, TILE_ROCK);
  for (int col = 2; col < CAVE_COLS - 2; col++) {
    for (int row = 3; row < GROUND_ROW; row++) put(g, col, row, TILE_EMPTY);
    put(g, col, GROUND_ROW, TILE_GRASS);
  }
  level->spawn_x = 5.0f * TILE; /* on the left pipe, where he arrives */
  level->spawn_y = (float)((GROUND_ROW - 2) * TILE);
  cave_pipe(g, 4, 2);
  cave_pipe(g, CAVE_COLS - 7, 2);

  /* Treasure: three prize blocks, one of them a rare weapon, coin trails and a high gold ledge. */
  int row = GROUND_ROW - 4;
  question_block(g, 13, row, rng_chance(&g->rng, 0.5f) ? ITEM_LIGHTSABER : ITEM_BUBBLE_GUN);
  put(g, 14, row, TILE_BRICK);
  question_block(g, 15, row, ITEM_POTION);
  put(g, 16, row, TILE_BRICK);
  question_block(g, 17, row, rng_chance(&g->rng, 0.5f) ? ITEM_PISTOL : ITEM_MUSHROOM);
  for (int arc = 0; arc < 3; arc++) {
    int start = 9 + arc * 11;
    for (int c = 0; c < 7; c++) add_item(g, ITEM_COIN, start + c, GROUND_ROW - 2 - (c == 0 || c == 6 ? 0 : (c == 3 ? 3 : 1)));
  }
  for (int c = 0; c < 9; c++) {
    put(g, 24 + c, GROUND_ROW - 7, TILE_GOLD);
    add_item(g, ITEM_COIN, 24 + c, GROUND_ROW - 8);
  }
  for (int step = 0; step < 3; step++) { /* a stone stair up to the ledge */
    for (int h = 1; h <= step + 1; h++) put(g, 21 + step, GROUND_ROW - h, TILE_STONE);
  }

  /* A guardian stands over the treasure. */
  level->guardian = rng_chance(&g->rng, 0.5f) ? 1 : 2;
  level->guardian_x = (float)(33 * TILE + TILE / 2);

  /* Crystals and mushrooms on the floor, stalactites from the ceiling. */
  for (int col = 8; col < CAVE_COLS - 8; col += 3) {
    if (rng_chance(&g->rng, 0.5f)) {
      int roll = rng_range(&g->rng, 0, 9);
      cave_decor(g, roll < 5 ? DECOR_CRYSTAL : (roll < 8 ? DECOR_MUSHROOM : DECOR_STALAGMITE), col,
                 rng_range(&g->rng, 0, 2), (float)(GROUND_ROW * TILE));
    }
  }
  for (int col = 3; col < CAVE_COLS - 3; col += 2)
    if (rng_chance(&g->rng, 0.7f))
      cave_decor(g, DECOR_STALACTITE, col, rng_range(&g->rng, 0, 4), (float)(3 * TILE));
}
