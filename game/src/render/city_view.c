#include "city_view.h"

#include <math.h>
#include <stdio.h>

#include "drawing_primitives.h"

#define OUTLINE_R 28
#define OUTLINE_G 22
#define OUTLINE_B 38
#define STOREY BALCONY_STEP

typedef struct {
  graphics_context_ptr gfx;
  const look_t* look;
  const world_t* world;
  float cam_x;
  float view_w;
} view_t;

static float ground_y(void) { return (float)(GROUND_ROW * TILE); }

/* A filled box in world coordinates. */
static void box(const view_t* v, float x, float y, float w, float h, int r, int g, int b) {
  draw_filled_rect_alpha(v->gfx, x - v->cam_x, y, w, h, COLOR((uint8_t)r, (uint8_t)g, (uint8_t)b), 255);
}

/* A box with the thick dark outline all the art has. */
static void solid(const view_t* v, float x, float y, float w, float h, int r, int g, int b) {
  box(v, x - 2.0f, y - 2.0f, w + 4.0f, h + 4.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  box(v, x, y, w, h, r, g, b);
}

static void text(const view_t* v, float cx, float y, const char* s, float scale, int r, int g, int b) {
  float w = pixel_text_width(s, scale);
  render_pixel_text(&v->look->font, v->gfx, cx - w * 0.5f - v->cam_x, y, s, scale, (uint8_t)r, (uint8_t)g, (uint8_t)b);
}

/* A sprite centred on `cx` with its bottom on `bottom`. */
static void sprite_at(const view_t* v, pose_t pose, float cx, float bottom, float scale, float alpha) {
  const sprite_t* s = look_sprite(v->look, pose);
  float w = (float)s->w * scale, h = (float)s->h * scale;
  draw_options_t options = {false, alpha, 0.0f, false, 0, 0, 0};
  render_sprite(v->gfx, &s->texture, NULL, (frect_t){cx - w * 0.5f - v->cam_x, bottom - h, w, h}, &options);
}

/* The same, facing left when `flip`. */
static void sprite_flipped(const view_t* v, pose_t pose, float cx, float bottom, bool flip, float alpha) {
  const sprite_t* s = look_sprite(v->look, pose);
  draw_options_t options = {flip, alpha, 0.0f, false, 0, 0, 0};
  render_sprite(v->gfx, &s->texture, NULL, (frect_t){cx - (float)s->w * 0.5f - v->cam_x, bottom - (float)s->h, (float)s->w, (float)s->h}, &options);
}

static bool on_screen(const view_t* v, float x, float w) { return x + w >= v->cam_x - 40.0f && x <= v->cam_x + v->view_w + 40.0f; }

/* ---- The street ------------------------------------------------------------------------ */

void city_draw_ground_tile(graphics_context_ptr gfx, float cam_x, float x, float y, bool surface, int col) {
  view_t v = {gfx, NULL, NULL, cam_x, 0.0f};
  if (surface) { /* paving stones with a pale kerb along the top */
    box(&v, x - 0.4f, y, TILE + 0.8f, TILE, 150, 144, 138);
    box(&v, x - 0.4f, y, TILE + 0.8f, 6.0f, 214, 208, 196);
    box(&v, x - 0.4f, y + 6.0f, TILE + 0.8f, 2.0f, OUTLINE_R + 30, OUTLINE_G + 30, OUTLINE_B + 30);
    float shift = (col & 1) ? 8.0f : 0.0f;
    box(&v, x + 10.0f + shift, y + 8.0f, 2.0f, 11.0f, 100, 96, 92);
    box(&v, x + 2.0f + shift * 0.5f, y + 19.0f, 2.0f, 13.0f, 100, 96, 92);
    box(&v, x + 22.0f - shift * 0.5f, y + 19.0f, 2.0f, 13.0f, 100, 96, 92);
    box(&v, x - 0.4f, y + 19.0f, TILE + 0.8f, 2.0f, 100, 96, 92);
  } else { /* the dark foundations, laid in bricks */
    box(&v, x - 0.4f, y, TILE + 0.8f, TILE, 88, 80, 96);
    box(&v, x - 0.4f, y + 15.0f, TILE + 0.8f, 2.0f, 56, 50, 66);
    box(&v, x - 0.4f, y + 31.0f, TILE + 0.8f, 2.0f, 56, 50, 66);
    box(&v, x + ((col & 1) ? 6.0f : 20.0f), y, 2.0f, 15.0f, 56, 50, 66);
    box(&v, x + ((col & 1) ? 22.0f : 6.0f), y + 17.0f, 2.0f, 14.0f, 56, 50, 66);
  }
}

/* ---- Signs ------------------------------------------------------------------------------ */

/* A road sign board with a pointed end: `dir` is -1 for an arrow to the left, +1 to the right. */
static void arrow_board(const view_t* v, float cx, float top, float w, float h, int dir, const char* label) {
  float left = cx - w * 0.5f;
  float body_left = dir < 0 ? left + h * 0.5f : left;
  float body_w = w - h * 0.5f;
  box(v, body_left - 2.0f, top - 2.0f, body_w + 4.0f, h + 4.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  box(v, body_left, top, body_w, h, 20, 82, 168);
  box(v, body_left + 3.0f, top + 3.0f, body_w - 6.0f, h - 6.0f, 20, 82, 168);
  for (int i = 0; i < (int)(h * 0.5f); i++) { /* the point */
    float tip = (float)i;
    float row_top = top + tip, row_h = h - 2.0f * tip;
    float x = dir < 0 ? left + h * 0.5f - tip - 2.0f : left + w - h * 0.5f + tip;
    box(v, x, row_top, 3.0f, row_h, i == 0 ? 20 : 20, 82, 168);
    box(v, x + (dir < 0 ? 0.0f : 0.0f), row_top - 2.0f, 3.0f, 2.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
    box(v, x, row_top + row_h, 3.0f, 2.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  }
  box(v, body_left, top, body_w, 2.0f, 235, 235, 245); /* a white edge along the top and bottom */
  box(v, body_left, top + h - 2.0f, body_w, 2.0f, 235, 235, 245);
  text(v, body_left + body_w * 0.5f, top + h * 0.5f - 6.0f, label, 2.0f, 255, 255, 255);
}

/* A filled disc, in horizontal slices. */
static void disc(const view_t* v, float cx, float cy, float r, int cr, int cg, int cb) {
  for (float dy = -r; dy <= r; dy += 1.0f) {
    float half = sqrtf(fmaxf(0.0f, r * r - dy * dy));
    box(v, cx - half, cy + dy, half * 2.0f, 1.0f, cr, cg, cb);
  }
}

/* A line of text shrunk to fit `max_w`. */
static void text_fit(const view_t* v, float cx, float y, const char* s, float max_w, float scale, int r, int g, int b) {
  float w = pixel_text_width(s, 1.0f);
  if (w * scale > max_w) scale = max_w / w;
  text(v, cx, y, s, scale, r, g, b);
}

/* Tiny flags for the signs: the Union Flag and the Italian tricolour. */
static void flag(const view_t* v, float x, float y, bool british) {
  solid(v, x, y, 36.0f, 24.0f, british ? 30 : 255, british ? 50 : 255, british ? 130 : 255);
  if (british) {
    box(v, x, y + 10.0f, 36.0f, 4.0f, 250, 250, 250);
    box(v, x + 16.0f, y, 4.0f, 24.0f, 250, 250, 250);
    box(v, x, y + 11.0f, 36.0f, 2.0f, 200, 30, 40);
    box(v, x + 17.0f, y, 2.0f, 24.0f, 200, 30, 40);
  } else {
    box(v, x, y, 12.0f, 24.0f, 0, 140, 70);
    box(v, x + 24.0f, y, 12.0f, 24.0f, 206, 43, 55);
  }
}

/* A slab of a ledge in front of a landmark, the top of which is what he stands on. */
static void slab(const view_t* v, float cx, float top, float w, int r, int g, int b) {
  solid(v, cx - w * 0.5f, top, w, 10.0f, r, g, b);
  box(v, cx - w * 0.5f, top, w, 3.0f, 255, 252, 240);
}

/* A cooked landmark, scaled to `height` pixels tall, standing on `ground`. */
static void landmark(const view_t* v, strip_id_t strip, float cx, float ground, float height) {
  const sprite_t* s = look_sprite(v->look, (pose_t){strip, 0});
  sprite_at(v, (pose_t){strip, 0}, cx, ground, height / (float)s->h, 1.0f);
}

/* A stone gallery with a balustrade, the top of which is what he stands on. */
static void gallery(const view_t* v, float cx, float top, float w, int r, int g, int b) {
  slab(v, cx, top, w, r, g, b);
  for (float x = cx - w * 0.5f + 6.0f; x < cx + w * 0.5f - 6.0f; x += 14.0f) box(v, x, top + 10.0f, 4.0f, 11.0f, r - 40, g - 40, b - 40);
  box(v, cx - w * 0.5f, top + 20.0f, w, 4.0f, r - 30, g - 30, b - 30);
}

static void draw_big_ben(const view_t* v, float cx, float ground) {
  landmark(v, STRIP_LM_BIG_BEN, cx, ground, 330.0f);
}

static void draw_bus(const view_t* v, float cx, float ground, int variant) {
  (void)variant;
  const sprite_t* s = look_sprite(v->look, (pose_t){STRIP_LM_BUS, 0});
  sprite_at(v, (pose_t){STRIP_LM_BUS, 0}, cx, ground, 180.0f / (float)s->w, 1.0f);
}

static void draw_abbey(const view_t* v, float cx, float ground) {
  landmark(v, STRIP_LM_ABBEY, cx, ground, 150.0f);
}

static void draw_biscuit_tin(const view_t* v, float cx, float ground) {
  const sprite_t* s = look_sprite(v->look, (pose_t){STRIP_LM_TIN, 0});
  sprite_at(v, (pose_t){STRIP_LM_TIN, 0}, cx, ground, 112.0f / (float)s->w, 1.0f);
}

static void draw_bull(const view_t* v, float cx, float ground) {
  if (v->world && v->world->bull.active) return; /* it has come alive and gone */
  sprite_at(v, (pose_t){STRIP_BULL_STATUE, 0}, cx, ground, 1.0f, 1.0f);
}

static void draw_rotunda(const view_t* v, float cx, float ground) {
  landmark(v, STRIP_LM_ROTUNDA, cx, ground, 330.0f);
  gallery(v, cx, ground - 190.0f, 116.0f, 186, 190, 200);
}

static void draw_duomo(const view_t* v, float cx, float ground) {
  landmark(v, STRIP_LM_DUOMO, cx, ground, 290.0f);
  for (int side = -1; side <= 1; side += 2) gallery(v, cx + (float)side * 120.0f, ground - 124.0f, 70.0f, 236, 226, 222);
  gallery(v, cx, ground - 218.0f, 120.0f, 236, 226, 222);
}

static void draw_tram(const view_t* v, float cx, float ground) {
  const sprite_t* s = look_sprite(v->look, (pose_t){STRIP_LM_TRAM, 0});
  sprite_at(v, (pose_t){STRIP_LM_TRAM, 0}, cx, ground, 210.0f / (float)s->w, 1.0f);
  box(v, cx - 112.0f, ground - 3.0f, 224.0f, 3.0f, 90, 86, 92); /* the rails */
}

static void draw_colosso(const view_t* v, float cx, float ground) {
  landmark(v, STRIP_LM_COLOSSO, cx, ground, 300.0f);
}

static void draw_castello(const view_t* v, float cx, float ground) {
  landmark(v, STRIP_LM_CASTELLO, cx, ground, 150.0f);
}

typedef struct {
  const char* name;
  uint8_t board[3], flagpole;
  bool british;
} city_sign_t;

static const city_sign_t kSigns[CITY_COUNT] = {
    {"LONDON", {28, 40, 100}, 0, true},     {"READING", {40, 90, 60}, 0, true},
    {"BIRMINGHAM", {96, 40, 60}, 0, true},  {"MILANO", {30, 96, 66}, 0, false},
    {"BARLETTA", {120, 40, 40}, 0, false},  {"SULMONA", {30, 96, 66}, 0, false},
};

static void draw_welcome(const view_t* v, const decor_t* d) {
  const city_sign_t* c = &kSigns[(d->variant >= 0 && d->variant < CITY_COUNT) ? d->variant : CITY_COUNT - 1];
  float ground = d->y;
  /* A wooden board on two posts, with a gold border and the flag of the country. */
  float w = 270.0f, h = 76.0f, top = ground - 160.0f;
  solid(v, d->x - w * 0.5f + 16.0f, top + h, 10.0f, 160.0f - h, 120, 80, 40);
  solid(v, d->x + w * 0.5f - 26.0f, top + h, 10.0f, 160.0f - h, 120, 80, 40);
  solid(v, d->x - w * 0.5f, top, w, h, c->board[0], c->board[1], c->board[2]);
  box(v, d->x - w * 0.5f + 5.0f, top + 5.0f, w - 10.0f, 3.0f, 240, 200, 70);
  box(v, d->x - w * 0.5f + 5.0f, top + h - 8.0f, w - 10.0f, 3.0f, 240, 200, 70);
  text(v, d->x, top + 14.0f, "WELCOME TO", 3.0f, 255, 246, 220);
  char name[24];
  snprintf(name, sizeof(name), "%s!", c->name);
  text_fit(v, d->x, top + 38.0f, name, w - 24.0f, 4.5f, 255, 214, 80);
  box(v, d->x + w * 0.5f - 12.0f, top - 28.0f, 3.0f, 28.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  flag(v, d->x + w * 0.5f - 9.0f, top - 28.0f, c->british);
}

bool city_draw_decor(graphics_context_ptr gfx, const look_t* look, const world_t* world, const decor_t* d, float cam_x) {
  if (d->kind < DECOR_WELCOME) return false;
  view_t v = {gfx, look, world, cam_x, 0.0f};
  float ground = d->y;
  switch (d->kind) {
    case DECOR_WELCOME: draw_welcome(&v, d); return true;
    case DECOR_BIG_BEN: draw_big_ben(&v, d->x, ground); return true;
    case DECOR_BUS: draw_bus(&v, d->x, ground, d->variant); return true;
    case DECOR_ABBEY: draw_abbey(&v, d->x, ground); return true;
    case DECOR_BISCUIT_TIN: draw_biscuit_tin(&v, d->x, ground); return true;
    case DECOR_BULL: draw_bull(&v, d->x, ground); return true;
    case DECOR_ROTUNDA: draw_rotunda(&v, d->x, ground); return true;
    case DECOR_DUOMO: draw_duomo(&v, d->x, ground); return true;
    case DECOR_TRAM: draw_tram(&v, d->x, ground); return true;
    case DECOR_COLOSSO: draw_colosso(&v, d->x, ground); return true;
    case DECOR_CASTELLO: draw_castello(&v, d->x, ground); return true;
    default: break;
  }
  if (d->kind != DECOR_SIGNPOST) return false;
  /* A signpost: Pescara to the left, L'Aquila to the right. */
  float post_top = ground - 170.0f;
  solid(&v, d->x - 5.0f, post_top, 10.0f, 170.0f, 110, 110, 120);
  arrow_board(&v, d->x - 4.0f, post_top + 10.0f, 150.0f, 34.0f, -1, "PESCARA");
  arrow_board(&v, d->x + 4.0f, post_top + 56.0f, 160.0f, 34.0f, 1, "L'AQUILA");
  return true;
}

/* ---- Buildings -------------------------------------------------------------------------- */

typedef struct {
  uint8_t wall[3], trim[3];
} palette_t;

static const palette_t kPalette[BUILDING_KIND_COUNT][3] = {
    /* apartments: sand, terracotta, mauve */
    {{{236, 202, 150}, {176, 120, 76}}, {{222, 126, 98}, {150, 76, 60}}, {{206, 176, 196}, {128, 96, 130}}},
    /* hotels: blue, cream, sage */
    {{{124, 164, 206}, {64, 92, 138}}, {{244, 226, 176}, {170, 140, 90}}, {{176, 206, 176}, {96, 130, 100}}},
    /* Thai massage shops: pinks */
    {{{244, 154, 196}, {170, 60, 120}}, {{250, 176, 150}, {180, 84, 70}}, {{214, 150, 226}, {120, 70, 150}}},
    /* shops: yellow, aqua, pale orange */
    {{{244, 206, 96}, {176, 130, 40}}, {{154, 206, 206}, {64, 130, 140}}, {{248, 190, 120}, {176, 100, 50}}},
    /* gelateria: cream, pistachio, strawberry */
    {{{250, 236, 200}, {214, 120, 150}}, {{196, 226, 170}, {110, 160, 90}}, {{248, 190, 200}, {190, 90, 120}}},
    /* barbiere: brick, navy-grey, ochre */
    {{{200, 110, 90}, {120, 56, 48}}, {{150, 170, 200}, {70, 90, 130}}, {{222, 186, 110}, {150, 112, 50}}},
    /* ristorante: terracotta, butter, olive */
    {{{232, 150, 100}, {160, 84, 50}}, {{250, 226, 150}, {190, 150, 70}}, {{190, 190, 130}, {110, 110, 70}}},
    /* supermercato: white-blue, pale green, light grey */
    {{{226, 234, 244}, {60, 110, 190}}, {{214, 238, 214}, {60, 150, 90}}, {{224, 224, 228}, {110, 110, 130}}},
    /* officina: concrete, steel blue, brick */
    {{{190, 190, 196}, {90, 90, 110}}, {{150, 176, 200}, {60, 90, 130}}, {{196, 130, 110}, {120, 70, 56}}},
    /* fioreria: lilac, mint, peach */
    {{{222, 196, 232}, {140, 90, 160}}, {{190, 232, 214}, {70, 150, 120}}, {{250, 206, 180}, {190, 110, 80}}},
    /* calzolaio: timber, sand, slate */
    {{{190, 150, 110}, {110, 76, 46}}, {{230, 210, 170}, {150, 120, 80}}, {{150, 160, 170}, {80, 90, 104}}},
};

static void shade(const uint8_t c[3], float k, int out[3]) {
  for (int i = 0; i < 3; i++) out[i] = (int)fminf(255.0f, (float)c[i] * k);
}

/* A window with a sill, and curtains if `lit`. */
static void window(const view_t* v, float x, float y, float w, float h, bool lit, const uint8_t trim[3]) {
  solid(v, x, y, w, h, lit ? 255 : 120, lit ? 226 : 176, lit ? 130 : 220);
  box(v, x + w * 0.5f - 1.0f, y, 2.0f, h, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  box(v, x, y + h * 0.45f, w, 2.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  if (!lit) box(v, x + 3.0f, y + 3.0f, w * 0.3f, h * 0.3f, 210, 236, 255);
  solid(v, x - 4.0f, y + h, w + 8.0f, 5.0f, trim[0], trim[1], trim[2]);
}

static void stripes_awning(const view_t* v, float x, float y, float w, int r, int g, int b) {
  box(v, x - 2.0f, y - 2.0f, w + 4.0f, 18.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  for (float s = 0.0f; s < w; s += 12.0f)
    box(v, x + s, y, fminf(12.0f, w - s), 14.0f, ((int)(s / 12.0f) & 1) ? 250 : r, ((int)(s / 12.0f) & 1) ? 250 : g,
        ((int)(s / 12.0f) & 1) ? 250 : b);
}

static void door(const view_t* v, float x, float w, float h, int r, int g, int b) {
  float ground = ground_y();
  solid(v, x, ground - h, w, h, r, g, b);
  box(v, x + w * 0.5f - 1.0f, ground - h, 2.0f, h, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  box(v, x + w - 8.0f, ground - h * 0.45f, 4.0f, 4.0f, 250, 214, 80);
}

static void balcony(const view_t* v, float cx, float bw, float floor_y, const uint8_t trim[3], bool flowers) {
  float left = cx - bw * 0.5f;
  if (flowers) sprite_at(v, (pose_t){STRIP_FLOWER_BOX, 0}, cx, floor_y, 0.75f, 1.0f);
  for (float p = 4.0f; p < bw - 2.0f; p += 14.0f) box(v, left + p, floor_y - 24.0f, 3.0f, 24.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  solid(v, left - 2.0f, floor_y - 28.0f, bw + 4.0f, 4.0f, trim[0], trim[1], trim[2]);
  solid(v, left - 4.0f, floor_y, bw + 8.0f, 11.0f, trim[0], trim[1], trim[2]);
  for (float p = 6.0f; p < bw - 6.0f; p += 24.0f) box(v, left + p, floor_y + 11.0f, 8.0f, 6.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
}


/* The shop's name on a board across the front, `y` from the street. */
static void shop_sign(const view_t* v, const building_t* b, float x, float w, int br, int bg, int bb, int tr, int tg, int tb) {
  float top = ground_y() - 78.0f;
  solid(v, x + 6.0f, top, w - 12.0f, 16.0f, br, bg, bb);
  text_fit(v, x + w * 0.5f, top + 1.0f, building_name(b->kind, b->english), w - 28.0f, 2.0f, tr, tg, tb);
}

static void street_shop(const view_t* v, const building_t* b, const palette_t* pal) {
  float ground = ground_y(), x = b->x, w = b->w, cx = x + w * 0.5f, t = v->world->time;
  switch (b->kind) {
    case BUILDING_GELATERIA: {
      shop_sign(v, b, x, w, 214, 96, 130, 255, 246, 226);
      stripes_awning(v, x + 8.0f, ground - 60.0f, w - 70.0f, 236, 120, 150);
      solid(v, x + 14.0f, ground - 40.0f, w - 82.0f, 34.0f, 255, 236, 200); /* the counter, with its tubs */
      for (float tx = x + 20.0f; tx < x + w - 82.0f; tx += 22.0f) {
        int tubs = (int)((tx - x) / 22.0f) % 3;
        solid(v, tx, ground - 36.0f, 18.0f, 12.0f, tubs == 0 ? 250 : (tubs == 1 ? 150 : 120), tubs == 0 ? 160 : (tubs == 1 ? 210 : 90), tubs == 0 ? 190 : (tubs == 1 ? 140 : 60));
      }
      door(v, x + w - 52.0f, 38.0f, 62.0f, 120, 170, 200);
      float kx = x + w - 28.0f; /* the cone on a bracket */
      for (int i = 0; i < 18; i++) box(v, kx - 9.0f + (float)i * 0.5f, ground - 118.0f + (float)i, 18.0f - (float)i, 1.0f, 214, 160, 80);
      disc(v, kx, ground - 124.0f, 10.0f, 246, 120, 170);
      disc(v, kx, ground - 134.0f, 8.0f, 250, 236, 200);
      break;
    }
    case BUILDING_BARBIERE: {
      shop_sign(v, b, x, w, 40, 50, 110, 250, 246, 226);
      window(v, x + 24.0f, ground - 60.0f, w - 96.0f, 40.0f, true, pal->trim);
      solid(v, x + 34.0f, ground - 42.0f, 22.0f, 20.0f, 190, 40, 50); /* the chair */
      box(v, x + 38.0f, ground - 54.0f, 14.0f, 14.0f, 190, 40, 50);
      door(v, x + w - 52.0f, 38.0f, 62.0f, 70, 90, 140);
      float px = x + 6.0f; /* the pole, turning */
      solid(v, px, ground - 58.0f, 12.0f, 40.0f, 250, 250, 250);
      int phase = (int)(t * 12.0f);
      for (int i = 0; i < 10; i++) {
        int c = (i + phase) % 4;
        if (c == 0) box(v, px, ground - 58.0f + (float)i * 4.0f, 12.0f, 4.0f, 200, 40, 50);
        if (c == 2) box(v, px, ground - 58.0f + (float)i * 4.0f, 12.0f, 4.0f, 40, 80, 190);
      }
      break;
    }
    case BUILDING_RISTORANTE: {
      shop_sign(v, b, x, w, 30, 96, 66, 255, 236, 190);
      stripes_awning(v, x + 8.0f, ground - 60.0f, w - 70.0f, 206, 43, 55);
      window(v, x + 18.0f, ground - 44.0f, w - 90.0f, 30.0f, true, pal->trim);
      door(v, x + w - 52.0f, 38.0f, 62.0f, 110, 70, 50);
      for (int i = 0; i < 2; i++) { /* a table and chairs outside, under a parasol */
        float tx = x + 34.0f + (float)i * (w - 150.0f) * 0.5f;
        if (w < 200.0f && i == 1) break;
        solid(v, tx - 14.0f, ground - 22.0f, 28.0f, 5.0f, 250, 244, 226);
        box(v, tx - 1.0f, ground - 17.0f, 3.0f, 17.0f, 90, 70, 60);
        solid(v, tx - 22.0f, ground - 24.0f, 6.0f, 24.0f, 140, 90, 50);
        solid(v, tx + 16.0f, ground - 24.0f, 6.0f, 24.0f, 140, 90, 50);
      }
      break;
    }
    case BUILDING_SUPERMERCATO: {
      shop_sign(v, b, x, w, 200, 40, 50, 255, 246, 226);
      solid(v, x + 12.0f, ground - 60.0f, w - 24.0f, 54.0f, 150, 210, 232); /* a long glass front */
      for (float gx = x + 24.0f; gx < x + w - 24.0f; gx += 40.0f) box(v, gx, ground - 60.0f, 2.0f, 54.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      for (float sx = x + 22.0f; sx < x + w - 40.0f; sx += 30.0f) { /* shelves */
        box(v, sx, ground - 36.0f, 22.0f, 3.0f, 120, 120, 130);
        box(v, sx + 2.0f, ground - 48.0f, 6.0f, 12.0f, 220, 90, 60);
        box(v, sx + 10.0f, ground - 46.0f, 6.0f, 10.0f, 250, 214, 80);
      }
      box(v, cx - 18.0f, ground - 58.0f, 36.0f, 52.0f, 170, 226, 244); /* the sliding doors */
      box(v, cx - 1.0f, ground - 58.0f, 2.0f, 52.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      solid(v, x + 14.0f, ground - 22.0f, 24.0f, 16.0f, 190, 190, 200); /* a trolley */
      box(v, x + 38.0f, ground - 30.0f, 8.0f, 3.0f, 190, 190, 200);
      disc(v, x + 20.0f, ground - 4.0f, 3.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      disc(v, x + 34.0f, ground - 4.0f, 3.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      break;
    }
    case BUILDING_OFFICINA: {
      shop_sign(v, b, x, w, 240, 200, 50, 50, 40, 30);
      solid(v, x + 14.0f, ground - 60.0f, w - 80.0f, 58.0f, 40, 36, 52); /* the bay */
      solid(v, x + 24.0f, ground - 38.0f, w - 100.0f, 22.0f, 190, 50, 50); /* a car on the ramp */
      solid(v, x + 40.0f, ground - 52.0f, w - 130.0f, 16.0f, 170, 40, 44);
      disc(v, x + 38.0f, ground - 16.0f, 7.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      disc(v, x + w - 96.0f, ground - 16.0f, 7.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      for (float sy = ground - 60.0f; sy < ground - 48.0f; sy += 4.0f) box(v, x + 12.0f, sy, w - 76.0f, 3.0f, 150, 152, 160); /* the shutter, half up */
      door(v, x + w - 50.0f, 36.0f, 60.0f, 90, 100, 120);
      for (int i = 0; i < 3; i++) disc(v, x + w - 70.0f, ground - 8.0f - (float)i * 11.0f, 8.0f, 50, 48, 56); /* tyres */
      break;
    }
    case BUILDING_FIORERIA: {
      shop_sign(v, b, x, w, 60, 150, 90, 255, 240, 250);
      window(v, x + 18.0f, ground - 60.0f, w - 90.0f, 34.0f, true, pal->trim);
      door(v, x + w - 52.0f, 38.0f, 62.0f, 90, 150, 110);
      for (int i = 0; i < 4; i++) { /* buckets of flowers outside */
        float bx = x + 16.0f + (float)i * 28.0f;
        solid(v, bx, ground - 18.0f, 20.0f, 18.0f, 160, 130, 90);
        for (int f = 0; f < 3; f++) {
          box(v, bx + 3.0f + (float)f * 6.0f, ground - 34.0f - (float)((i + f) % 3) * 4.0f, 2.0f, 16.0f, 50, 130, 60);
          int c = (i + f) % 4;
          disc(v, bx + 4.0f + (float)f * 6.0f, ground - 36.0f - (float)((i + f) % 3) * 4.0f, 4.0f, c == 0 ? 230 : (c == 1 ? 250 : (c == 2 ? 200 : 250)), c == 0 ? 60 : (c == 1 ? 210 : (c == 2 ? 90 : 120)), c == 0 ? 90 : (c == 1 ? 60 : (c == 2 ? 220 : 170)));
        }
      }
      break;
    }
    case BUILDING_CALZOLAIO: {
      shop_sign(v, b, x, w, 110, 76, 46, 250, 226, 170);
      window(v, x + 18.0f, ground - 60.0f, w - 90.0f, 46.0f, true, pal->trim);
      for (float sx = x + 26.0f; sx < x + w - 80.0f; sx += 26.0f) { /* shoes on a shelf */
        box(v, sx, ground - 34.0f, 22.0f, 3.0f, 100, 70, 50);
        solid(v, sx, ground - 44.0f, 16.0f, 9.0f, 150, 90, 50);
        box(v, sx + 12.0f, ground - 50.0f, 6.0f, 6.0f, 150, 90, 50);
      }
      door(v, x + w - 52.0f, 38.0f, 62.0f, 100, 70, 50);
      float bx = x + w - 20.0f; /* a giant boot hanging out over the pavement */
      solid(v, bx - 8.0f, ground - 126.0f, 10.0f, 26.0f, 150, 90, 50);
      solid(v, bx - 8.0f, ground - 104.0f, 26.0f, 10.0f, 150, 90, 50);
      box(v, bx - 8.0f, ground - 96.0f, 26.0f, 3.0f, 70, 46, 30);
      break;
    }
    default: break;
  }
}

static void draw_building(const view_t* v, const building_t* b) {
  const palette_t* pal = &kPalette[b->kind][b->variant % 3];
  float ground = ground_y();
  float top = ground - (float)b->floors * STOREY;
  float x = b->x, w = b->w, h = (float)b->floors * STOREY;
  float cx = x + w * 0.5f;
  int wall_shade[3];
  shade(pal->wall, 0.84f, wall_shade);

  solid(v, x, top, w, h, pal->wall[0], pal->wall[1], pal->wall[2]);
  box(v, x + w - 12.0f, top, 12.0f, h, wall_shade[0], wall_shade[1], wall_shade[2]);
  for (int f = 1; f < b->floors; f++) box(v, x, ground - (float)f * STOREY - 4.0f, w, 5.0f, pal->trim[0], pal->trim[1], pal->trim[2]);

  /* The upper storeys: two or three windows each, lit in a pattern, with a flower-box balcony between. */
  int across = (int)(w / 72.0f);
  if (across < 2) across = 2;
  for (int f = 1; f < b->floors; f++) {
    float floor_y = ground - (float)f * STOREY;
    for (int i = 0; i < across; i++) {
      float wx = x + w / (float)across * ((float)i + 0.5f) - 14.0f;
      if (b->kind == BUILDING_THAI && f == 1 && i == 0) continue; /* a masseuse leans out of this one */
      window(v, wx, floor_y - 82.0f, 28.0f, 52.0f, ((i * 7 + f * 3 + (int)(x / TILE)) % 4) != 0, pal->trim);
    }
  }
  /* The storey above the shopfronts of the two-storey buildings, under a roof of the same colour. */
  if (b->floors == 2) {
    float floor_y = ground - STOREY;
    float roof_y = top;
    (void)floor_y;
    (void)roof_y;
  }

  /* The street storey. */
  switch (b->kind) {
    case BUILDING_APARTMENT:
      door(v, cx - 18.0f, 36.0f, 66.0f, 110, 70, 50);
      window(v, x + 22.0f, ground - 78.0f, 28.0f, 40.0f, true, pal->trim);
      window(v, x + w - 52.0f, ground - 78.0f, 28.0f, 40.0f, false, pal->trim);
      break;
    case BUILDING_HOTEL:
      door(v, cx - 26.0f, 52.0f, 68.0f, 70, 60, 90);
      box(v, cx - 22.0f, ground - 62.0f, 20.0f, 58.0f, 150, 210, 230); /* glass doors */
      box(v, cx + 4.0f, ground - 62.0f, 18.0f, 58.0f, 150, 210, 230);
      stripes_awning(v, cx - 44.0f, ground - 86.0f, 88.0f, 200, 40, 50);
      window(v, x + 16.0f, ground - 76.0f, 28.0f, 40.0f, true, pal->trim);
      window(v, x + w - 44.0f, ground - 76.0f, 28.0f, 40.0f, true, pal->trim);
      break;
    case BUILDING_THAI: {
      /* A shopfront: the masseuse at the window under a pink awning, the door, the neon sign. */
      door(v, x + w - 54.0f, 38.0f, 58.0f, 120, 36, 90);
      int window_kind = ((int)(x / TILE)) & 1;
      sprite_at(v, pose_masseuse(window_kind, v->world->time + x * 0.013f), x + 56.0f, ground - 8.0f, 0.82f, 1.0f);
      int frame = ((int)(v->world->time * 3.0f + x * 0.01f)) % 7 == 0 ? 2 : 0; /* the neon flickers now and then */
      sprite_at(v, (pose_t){STRIP_THAI_SIGN, frame + (((int)(v->world->time * 6.0f)) % 5 == 0 ? 1 : 0)}, x + w - 35.0f, ground - 60.0f, 0.62f, 1.0f);
      sprite_at(v, (pose_t){STRIP_LANTERN, (int)(v->world->time * 2.0f + x)}, x + 14.0f, ground - STOREY + 48.0f, 0.8f, 1.0f);
      sprite_at(v, (pose_t){STRIP_LANTERN, (int)(v->world->time * 2.0f + x + 1.0f)}, x + w - 14.0f, ground - STOREY + 48.0f, 0.8f, 1.0f);
      /* ... and a second masseuse at the window above. */
      sprite_at(v, pose_masseuse(1 - window_kind, v->world->time * 1.1f + x * 0.007f + 0.5f), x + 46.0f, ground - STOREY - 10.0f, 0.8f, 1.0f);
      break;
    }
    case BUILDING_GELATERIA:
    case BUILDING_BARBIERE:
    case BUILDING_RISTORANTE:
    case BUILDING_SUPERMERCATO:
    case BUILDING_OFFICINA:
    case BUILDING_FIORERIA:
    case BUILDING_CALZOLAIO:
      street_shop(v, b, pal);
      break;
    case BUILDING_SHOP:
    default:
      window(v, x + 18.0f, ground - 74.0f, w - 90.0f, 46.0f, true, pal->trim);
      stripes_awning(v, x + 8.0f, ground - 88.0f, w - 70.0f, 40, 150, 90);
      door(v, x + w - 52.0f, 38.0f, 64.0f, 70, 90, 120);
      break;
  }

  /* Balconies, in front of the windows. */
  for (int f = 1; f < b->floors; f++) {
    float bw = fminf(w - 2.0f * TILE, 4.0f * TILE);
    balcony(v, cx, bw, ground - (float)f * STOREY, pal->trim, (b->variant + f) % 2 == 0);
  }

  /* The roof's edge, which can be walked on, and what stands on it. */
  solid(v, x - 6.0f, top, w + 12.0f, 12.0f, pal->trim[0], pal->trim[1], pal->trim[2]);
  box(v, x - 4.0f, top, w + 8.0f, 3.0f, 250, 244, 226);
  switch (b->variant % 3) {
    case 0: /* a chimney */
      solid(v, x + w * 0.2f, top - 28.0f, 18.0f, 28.0f, 170, 90, 70);
      box(v, x + w * 0.2f - 3.0f, top - 32.0f, 24.0f, 6.0f, 90, 70, 70);
      break;
    case 1: /* a TV aerial */
      box(v, x + w * 0.7f, top - 34.0f, 3.0f, 34.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      box(v, x + w * 0.7f - 10.0f, top - 30.0f, 23.0f, 3.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      box(v, x + w * 0.7f - 6.0f, top - 22.0f, 15.0f, 3.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      break;
    default: /* a water tank on legs */
      box(v, x + w * 0.3f, top - 12.0f, 3.0f, 12.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      box(v, x + w * 0.3f + 26.0f, top - 12.0f, 3.0f, 12.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
      solid(v, x + w * 0.3f - 2.0f, top - 34.0f, 34.0f, 22.0f, 160, 120, 90);
      break;
  }
  if (b->kind == BUILDING_HOTEL) { /* a sign on the roof */
    float sw = w * 0.62f;
    solid(v, cx - sw * 0.5f, top - 44.0f, sw, 32.0f, 24, 30, 70);
    box(v, cx - sw * 0.5f + 3.0f, top - 41.0f, sw - 6.0f, 2.0f, 250, 214, 80);
    text(v, cx, top - 33.0f, "HOTEL", 3.0f, 255, 224, 110);
  }
}

static void draw_aqueduct(const view_t* v) {
  const level_t* level = &v->world->level;
  if (!level->has_aqueduct) return;
  float ground = ground_y();
  float x = level->aqueduct_x0;
  while (x < level->aqueduct_x1 - 1.0f) {
    const sprite_t* s = look_sprite(v->look, (pose_t){STRIP_AQUEDUCT, 0});
    draw_options_t options = {false, 1.0f, 0.0f, false, 0, 0, 0};
    if (on_screen(v, x, (float)s->w))
      render_sprite(v->gfx, &s->texture, NULL, (frect_t){x - v->cam_x, ground + 2.0f - (float)s->h, (float)s->w - 1.0f, (float)s->h}, &options);
    x += (float)s->w - 1.0f;
  }
}


/* ---- The London Eye ------------------------------------------------------------------------- */

static void draw_eye(const view_t* v) {
  const level_t* level = &v->world->level;
  if (!level->has_eye || !on_screen(v, level->eye_x - EYE_RADIUS - 60.0f, EYE_RADIUS * 2.0f + 400.0f)) return;
  float ground = ground_y(), hx = level->eye_x - v->cam_x, hy = level->eye_y;
  color_t steel = COLOR(226, 232, 242), shadow = COLOR(OUTLINE_R, OUTLINE_G, OUTLINE_B);
  /* the A-frame it leans on */
  draw_thick_line(v->gfx, (int)hx, (int)hy, (int)(hx - 64.0f), (int)ground, shadow);
  draw_thick_line(v->gfx, (int)hx, (int)hy, (int)(hx + 64.0f), (int)ground, shadow);
  draw_thick_line(v->gfx, (int)hx + 2, (int)hy, (int)(hx - 62.0f), (int)ground, COLOR(120, 130, 150));
  draw_thick_line(v->gfx, (int)hx + 2, (int)hy, (int)(hx + 66.0f), (int)ground, COLOR(120, 130, 150));
  /* the rim and the spokes */
  for (int r = -3; r <= 3; r++) draw_circle(v->gfx, (int)hx, (int)hy, (int)EYE_RADIUS + r, (r == -3 || r == 3) ? shadow : steel);
  draw_circle(v->gfx, (int)hx, (int)hy, (int)EYE_RADIUS - 8, steel);
  for (int i = 0; i < 24; i++) {
    float a = (float)i * 0.2618f + v->world->time * 0.02f;
    draw_line(v->gfx, (int)hx, (int)hy, (int)(hx + cosf(a) * EYE_RADIUS), (int)(hy + sinf(a) * EYE_RADIUS), COLOR(190, 200, 214));
  }
  const sprite_t* hub = look_sprite(v->look, (pose_t){STRIP_LM_HUB, 0});
  sprite_at(v, (pose_t){STRIP_LM_HUB, 0}, level->eye_x, hy + 32.0f, 64.0f / (float)hub->w, 1.0f);
  /* the capsules, each one level: a glass egg on the rim */
  for (int i = 0; i < EYE_PODS; i++) {
    float px, floor_y;
    platform_position(&level->platforms[level->eye_first_pod + i], v->world->time, &px, &floor_y);
    const sprite_t* pod = look_sprite(v->look, (pose_t){STRIP_LM_POD, 0});
    sprite_at(v, (pose_t){STRIP_LM_POD, 0}, px, floor_y + 4.0f, (POD_W + 14.0f) / (float)pod->w, 1.0f);
  }
  /* the viewing deck on its pillars */
  float tx0 = level->terrace_x0, tx1 = level->terrace_x1, ty = level->terrace_y;
  if (tx1 > tx0) {
    for (float x = tx0 + 10.0f; x < tx1 - 8.0f; x += 60.0f) solid(v, x, ty + 10.0f, 14.0f, ground - ty - 10.0f, 182, 176, 166);
    slab(v, (tx0 + tx1) * 0.5f, ty, tx1 - tx0, 176, 170, 160);
    for (float x = tx0 + 4.0f; x < tx1 - 4.0f; x += 16.0f) box(v, x, ty - 24.0f, 3.0f, 24.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
    box(v, tx0, ty - 26.0f, tx1 - tx0, 3.0f, OUTLINE_R, OUTLINE_G, OUTLINE_B);
  }
}

void city_draw_back(graphics_context_ptr gfx, const look_t* look, const world_t* world, float cam_x, float view_w) {
  const level_t* level = &world->level;
  if (level->cave) return;
  view_t v = {gfx, look, world, cam_x, view_w};
  for (int i = 0; i < level->building_count; i++)
    if (on_screen(&v, level->buildings[i].x, level->buildings[i].w)) draw_building(&v, &level->buildings[i]);
  draw_eye(&v);
  draw_aqueduct(&v);
}

/* ---- Foreground: the tap, the water, the vases ------------------------------------------ */

void city_draw_front(graphics_context_ptr gfx, const look_t* look, const world_t* world, float cam_x, float view_w) {
  const level_t* level = &world->level;
  if (level->cave) return;
  view_t v = {gfx, look, world, cam_x, view_w};
  float ground = ground_y();
  float t = world->time;

  for (int i = 0; i < level->vase_count; i++) {
    const vase_t* vase = &world->vases[i];
    if (vase->state == VASE_SMASHED || !on_screen(&v, level->vase_x[i], 40.0f)) continue;
    if (vase->state == VASE_STANDING) {
      sprite_at(&v, (pose_t){STRIP_VASE, 0}, level->vase_x[i], level->vase_y[i], 1.0f, 1.0f);
    } else {
      draw_options_t options = {false, 1.0f, vase->vy * 0.08f, false, 0, 0, 0};
      const sprite_t* s = look_sprite(look, (pose_t){STRIP_VASE, 1});
      render_sprite(gfx, &s->texture, NULL,
                    (frect_t){vase->x - (float)s->w * 0.5f - cam_x, vase->y - (float)s->h, (float)s->w, (float)s->h}, &options);
    }
  }

  const bull_t* bull = &world->bull;
  if (bull->active && on_screen(&v, bull->x - 200.0f, 400.0f)) {
    bool flip = bull->dir < 0;
    if (bull->age < BULL_REAR_SECONDS) { /* the statue comes alive */
      int frame = (int)(bull->age / BULL_REAR_SECONDS * 3.0f);
      sprite_flipped(&v, (pose_t){STRIP_BULL_APPEAR, frame}, bull->x, ground, flip, 1.0f);
    } else {
      sprite_flipped(&v, (pose_t){STRIP_BULL_DUST, (int)(t * 12.0f)}, bull->x - (float)bull->dir * 130.0f, ground + 2.0f, flip, 0.85f);
      sprite_flipped(&v, (pose_t){STRIP_BULL_RUN, (int)(t * 16.0f)}, bull->x, ground + 2.0f, flip, 1.0f);
    }
  }

  if (!level->has_aqueduct) return;
  bool running = water_running(world);
  float fade = running ? fminf(1.0f, world->water_left / 1.0f) : 0.0f; /* thins away for the last second */
  if (on_screen(&v, level->tap_x, 40.0f)) {
    int frame = running ? 2 + ((int)(t * 6.0f) & 1) : 0;
    sprite_at(&v, (pose_t){STRIP_TAP, frame}, level->tap_x, ground + 1.0f, 1.3f, 1.0f);
    if (!running && fabsf(world->player.x - level->tap_x) < 260.0f && world->player.alive) {
      float bob = sinf(t * 5.0f) * 3.0f;
      text(&v, level->tap_x, ground - 96.0f + bob, "UP: WATER", 2.0f, 255, 255, 255);
    }
  }
  if (!running) return;
  for (int i = 0; i < level->spout_count; i++) {
    float x = level->spout_x[i];
    if (!on_screen(&v, x, 60.0f)) continue;
    float top = level->aqueduct_deck + 36.0f;
    const sprite_t* s = look_sprite(look, (pose_t){STRIP_WATER, (int)(t * 10.0f + (float)i)});
    draw_options_t options = {false, 0.9f * fade + 0.05f, 0.0f, false, 0, 0, 0};
    render_sprite(gfx, &s->texture, NULL, (frect_t){x - 24.0f - cam_x, top, 48.0f, ground - top}, &options);
    sprite_at(&v, (pose_t){STRIP_SPLASH, (int)(t * 8.0f + (float)i)}, x, ground + 2.0f, 1.5f, 0.9f * fade + 0.05f);
  }
}
