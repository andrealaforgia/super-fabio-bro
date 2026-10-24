#include "level.h"

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
float level_height(void) { return (float)(LEVEL_ROWS * TILE); }

typedef struct {
  level_t* level;
  rng_t rng;
  int x;               /* next free column */
  int since_hazard;    /* flat columns since the last pit, pipe or stairs */
  int since_potion;    /* columns since a potion was made available */
  int since_pistol;
  int last_decor;
  int counted_x;       /* columns already added to the since_* counters */
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
  decor->x = (float)(col * TILE + TILE / 2);
  decor->y = (float)(GROUND_ROW * TILE);
  g->last_decor = col;
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
      add_decor(g, roll < 5 ? DECOR_PINE : (roll < 8 ? DECOR_OAK : DECOR_BUSH),
                g->x);
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

static item_kind_t roll_content(generator_t* g) {
  if (g->since_potion > 45 || rng_chance(&g->rng, 0.1f)) {
    g->since_potion = 0;
    return ITEM_POTION;
  }
  if (g->since_pistol > 110 || rng_chance(&g->rng, 0.04f)) {
    g->since_pistol = 0;
    return ITEM_PISTOL;
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
  g->x += width;
  g->since_hazard = 0;
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

  level->spawn_x = 3.0f * TILE;
  level->spawn_y = (float)(GROUND_ROW * TILE);

  /* The opening: room to move, then a row of bricks with the pistol and a
   * potion in the first question blocks. */
  flat(g, 9, true);
  int left = g->x;
  for (int i = 0; i < 9; i++) ground_column(g, left + i);
  int row = GROUND_ROW - 4;
  put(g, left + 1, row, TILE_BRICK);
  question_block(g, left + 2, row, ITEM_PISTOL);
  put(g, left + 3, row, TILE_BRICK);
  question_block(g, left + 4, row, ITEM_POTION);
  put(g, left + 5, row, TILE_BRICK);
  add_item(g, ITEM_COIN, left + 7, GROUND_ROW - 2);
  g->x += 9;
  g->since_potion = 0;
  g->since_pistol = 0;

  g->counted_x = g->x;
  while (g->x < LEVEL_COLS - 52) {
    g->since_potion += g->x - g->counted_x;
    g->since_pistol += g->x - g->counted_x;
    g->counted_x = g->x;
    if (g->since_hazard < 6) {
      flat(g, rng_range(&g->rng, 3, 6), true);
      continue;
    }
    int roll = rng_range(&g->rng, 0, 99);
    if (g->since_potion > 60) roll = 30; /* a platform, and a potion on it */
    if (roll < 22) {
      flat(g, rng_range(&g->rng, 6, 14), true);
    } else if (roll < 50) {
      platform(g, rng_range(&g->rng, 3, 8));
    } else if (roll < 66) {
      pit(g, rng_range(&g->rng, 2, 6));
      flat(g, 6, false);
    } else if (roll < 82) {
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
