/**
 * @file ally_path.c
 * @brief How a friend gets from the floor he stands on to the one Fabio is on
 *
 * Fabio jumps up onto balconies, roofs and blocks, and his friends must follow
 * the way he would: walking to a good spot, jumping onto the next thing that is
 * high enough, and so on up (or walking off the edge to come down). Nothing here
 * moves anybody; it names the next place to walk to, and whether to jump there.
 *
 * The floors near the two of them are gathered into a short list (runs of tile
 * tops, and every plank where it is at this moment), two floors are joined when a jump or a
 * step off an edge gets from one to the other, and a breadth-first search finds
 * the shortest chain. It is a plan, not a promise: the friend asks again every
 * quarter of a second, from wherever he has got to.
 */

#include <math.h>

#include "world.h"

#define MAX_SURFACES 256
#define MAX_RISE 140.0f      /* the highest step up a jump clears (the apex is about 156) */
#define MAX_STEP_OFF 60.0f   /* how far from an edge a drop still lands on the floor below */
#define EDGE_INSET 8.0f
#define JUMP_RUN (RUN_SPEED * 1.2f)
#define TAKE_OFF_STEP 20.0f  /* spacing of the take-off spots tried along a floor */
#define TAKE_OFF_WINDOW 300.0f

typedef struct {
  float x0, x1; /* the stretch he can stand on */
  float top;    /* where his feet rest */
  int plank;    /* the plank, or -1 for tiles */
} surface_t;

typedef struct {
  surface_t list[MAX_SURFACES];
  int count;
} surfaces_t;

typedef struct {
  bool ok;
  float dep;  /* where to take off (or step off) */
  float land; /* where to head for once in the air */
  bool jump;
} edge_t;

static bool add_surface(surfaces_t* s, float x0, float x1, float top, int plank) {
  if (s->count >= MAX_SURFACES) return false; /* more floors than fit: the farthest are left out */
  s->list[s->count++] = (surface_t){x0, x1, top, plank};
  return true;
}

static bool solid(const level_t* level, int col, int row) { return tile_is_solid(level_tile(level, col, row)); }

/* Runs of tile tops with room for him to stand above them. */
static void collect_tiles(const level_t* level, int first_col, int last_col, surfaces_t* out) {
  for (int col = first_col; col <= last_col; col++) {
    for (int row = 1; row < LEVEL_ROWS; row++) {
      if (!solid(level, col, row) || solid(level, col, row - 1) || solid(level, col, row - 2)) continue;
      float top = (float)(row * TILE), left = (float)(col * TILE);
      bool extended = false;
      for (int i = out->count - 1; i >= 0 && i >= out->count - 80; i--) {
        surface_t* s = &out->list[i];
        if (s->plank < 0 && s->top == top && s->x1 == left) {
          s->x1 += (float)TILE;
          extended = true;
          break;
        }
      }
      if (!extended) add_surface(out, left, left + (float)TILE, top, -1);
    }
  }
}

static bool has_plank(const surfaces_t* s, int plank) {
  for (int i = 0; i < s->count; i++)
    if (s->list[i].plank == plank) return true;
  return false;
}

static void add_plank(const world_t* world, surfaces_t* out, int i) {
  const plank_t* plank = &world->planks[i];
  if (plank->off || has_plank(out, i)) return;
  float half = world->level.platforms[i].w * 0.5f;
  add_surface(out, plank->x - half, plank->x + half, plank->top, i);
}

/* Every plank where it is at this moment, the moving ones too. */
static void collect_planks(const world_t* world, float from_x, float to_x, surfaces_t* out) {
  const level_t* level = &world->level;
  for (int i = 0; i < level->platform_count; i++) {
    const plank_t* plank = &world->planks[i];
    float half = level->platforms[i].w * 0.5f;
    if (plank->x + half < from_x || plank->x - half > to_x) continue;
    add_plank(world, out, i);
  }
}

static int surface_of(const surfaces_t* s, float x, float feet, int plank) {
  if (plank >= 0)
    for (int i = 0; i < s->count; i++)
      if (s->list[i].plank == plank) return i;
  for (int i = 0; i < s->count; i++) {
    const surface_t* f = &s->list[i];
    if (f->plank < 0 && x >= f->x0 - 2.0f && x <= f->x1 + 2.0f && fabsf(f->top - feet) <= 4.0f) return i;
  }
  return -1;
}

static float clamp_in(float x, float lo, float hi) {
  if (hi < lo) return (lo + hi) * 0.5f;
  return x < lo ? lo : (x > hi ? hi : x);
}

/* Whether his body, feet at (x, feet), is inside any solid tile. */
static bool body_in_wall(const level_t* level, float x, float feet) {
  int c0 = (int)floorf((x - ALLY_W * 0.5f + 0.5f) / (float)TILE), c1 = (int)floorf((x + ALLY_W * 0.5f - 0.5f) / (float)TILE);
  int r0 = (int)floorf((feet - ALLY_H + 0.5f) / (float)TILE), r1 = (int)floorf((feet - 0.5f) / (float)TILE);
  for (int r = r0; r <= r1; r++)
    for (int c = c0; c <= c1; c++)
      if (solid(level, c, r)) return true;
  return false;
}

/* Plays a jump through with the very rules he moves by (the same take-off speed, gravity and steering
 * towards `land`) and says whether he comes down on top of `b` without touching anything solid on the
 * way: ceilings, walls, the side of the block he means to land on. Planks are one-way, so they are
 * passed through on the way up. */
static bool jump_works(const level_t* level, float dep, float feet, float land, const surface_t* b) {
  const float dt = 1.0f / 120.0f;
  float x = dep, y = feet, vy = -JUMP_SPEED, vx = (land >= dep ? 1.0f : -1.0f) * RUN_SPEED;
  for (int step = 0; step < 240; step++) {
    float want = clampf((land - x) * 5.0f, -JUMP_RUN, JUMP_RUN);
    if (fabsf(land - x) < 6.0f) want = 0.0f;
    vx = approachf(vx, want, 3200.0f * dt);
    float before = y;
    vy = fminf(vy + GRAVITY * dt, MAX_FALL);
    y += vy * dt;
    x += vx * dt;
    if (vy > 0.0f && before <= b->top + 6.0f && y >= b->top - 0.5f) return x >= b->x0 + 2.0f && x <= b->x1 - 2.0f;
    if (body_in_wall(level, x, y)) return false;
  }
  return false;
}

#define APEX (JUMP_SPEED * JUMP_SPEED / (2.0f * GRAVITY))
#define TIME_UP (JUMP_SPEED / GRAVITY)

/* The farthest a jump to a floor `h` higher could ever carry him: a cheap test before the playing-through. */
static float reach_bound(float h) {
  return JUMP_RUN * (TIME_UP + sqrtf(fmaxf(0.0f, 2.0f * (APEX - fmaxf(h, 0.0f)) / GRAVITY))) + 40.0f;
}

/* Whether a friend on `a` can get to `b` in one move, and how. `from_x` is where he is (or will be) on
 * `a`: of the ways that work, the one nearest to it is chosen. */
static edge_t edge(const level_t* level, const surface_t* a, const surface_t* b, float from_x) {
  edge_t none = {false, 0.0f, 0.0f, false};
  float gap = fmaxf(0.0f, fmaxf(b->x0 - a->x1, a->x0 - b->x1));
  float rise = a->top - b->top; /* positive: b is higher */
  float a_lo = a->x0 + EDGE_INSET, a_hi = a->x1 - EDGE_INSET;

  if (rise < -2.0f) { /* b is lower: walk off an edge of `a` that has `b` beyond it */
    edge_t best = none;
    float best_cost = 1e9f;
    for (int right = 0; right < 2; right++) {
      bool beyond = right ? b->x1 > a->x1 + 16.0f : b->x0 < a->x0 - 16.0f;
      float reach = right ? b->x0 - a->x1 : a->x0 - b->x1; /* positive: a hole between */
      if (!beyond || reach > MAX_STEP_OFF) continue;
      edge_t e = {true, right ? a_hi : a_lo, 0.0f, false};
      e.land = right ? clamp_in(a->x1 + 24.0f, b->x0 + 12.0f, b->x1 - 12.0f) : clamp_in(a->x0 - 24.0f, b->x0 + 12.0f, b->x1 - 12.0f);
      float cost = fabsf(e.dep - from_x);
      if (cost < best_cost) {
        best_cost = cost;
        best = e;
      }
    }
    (void)gap;
    return best;
  }
  if (rise > MAX_RISE || gap > reach_bound(rise)) return none;

  /* A jump: try take-off spots along `a` near `b` and keep the best that works. */
  float inset = fminf(24.0f, (b->x1 - b->x0) * 0.5f);
  float lo = fmaxf(a_lo, b->x0 - TAKE_OFF_WINDOW), hi = fminf(a_hi, b->x1 + TAKE_OFF_WINDOW);
  edge_t best = none;
  float best_cost = 1e9f;
  for (float dep = lo; dep <= hi + 0.5f; dep += TAKE_OFF_STEP) {
    float land = clamp_in(dep, b->x0 + inset, b->x1 - inset);
    float cost = fabsf(dep - from_x);
    if (cost >= best_cost) continue;
    if (!jump_works(level, dep, a->top, land, b)) continue;
    best_cost = cost;
    best.ok = true;
    best.dep = dep;
    best.land = land;
    best.jump = true;
  }
  if (!best.ok && hi < lo) return none;
  return best;
}

ally_step_t ally_plan_step(const world_t* world, const ally_t* ally) {
  ally_step_t none = {false, 0.0f, 0.0f, false};
  const player_t* p = &world->player;
  const level_t* level = &world->level;
  float lo = fminf(ally->x, p->x), hi = fmaxf(ally->x, p->x);
  int first = (int)floorf(lo / (float)TILE) - 16, last = (int)floorf(hi / (float)TILE) + 16;
  if (first < 0) first = 0;
  if (last >= LEVEL_COLS) last = LEVEL_COLS - 1;

  /* One plan at a time, made and used at once: the list lives in a buffer of its own (not re-entrant). */
  static surfaces_t surfaces;
  surfaces.count = 0;
  /* The floors they stand on come first, so that they are always in the list. */
  if (ally->platform >= 0) add_plank(world, &surfaces, ally->platform);
  if (p->platform >= 0) add_plank(world, &surfaces, p->platform);
  collect_tiles(level, first, last, &surfaces);
  collect_planks(world, (float)(first * TILE), (float)((last + 1) * TILE), &surfaces);

  int start = surface_of(&surfaces, ally->x, ally->y, ally->platform);
  int goal = surface_of(&surfaces, p->x, p->y, p->platform);
  if (start < 0 || goal < 0 || start == goal) return none;

  int queue[MAX_SURFACES];
  float arrive[MAX_SURFACES];
  edge_t first_hop[MAX_SURFACES];
  bool seen[MAX_SURFACES] = {false};
  int head = 0, tail = 0;
  queue[tail++] = start;
  seen[start] = true;
  arrive[start] = ally->x;
  while (head < tail && !seen[goal]) {
    int u = queue[head++];
    for (int v = 0; v < surfaces.count; v++) {
      if (seen[v]) continue;
      edge_t e = edge(level, &surfaces.list[u], &surfaces.list[v], arrive[u]);
      if (!e.ok) continue;
      seen[v] = true;
      arrive[v] = e.land;
      first_hop[v] = u == start ? e : first_hop[u];
      queue[tail++] = v;
    }
  }
  if (!seen[goal]) return none;
  ally_step_t step = {true, first_hop[goal].dep, first_hop[goal].land, first_hop[goal].jump};
  return step;
}
