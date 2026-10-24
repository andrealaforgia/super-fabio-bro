#include "render.h"

#include <math.h>
#include <stdio.h>

#define PI 3.14159265f
#define FABIO_SCALE 0.5f
#define ZOMBIE_SCALE 0.46f

typedef struct {
  gfx_t* gfx;
  const look_t* look;
  const world_t* world;
  float cam_x;
  float view_w;
} frame_t;

static rect_t rect(float x, float y, float w, float h) {
  rect_t r = {x, y, w, h};
  return r;
}

/* Feet-anchored sprite. */
static void draw_sprite(const frame_t* f, pose_t pose, float x, float y,
                        float scale, bool flip, float alpha) {
  const sprite_t* sprite = look_sprite(f->look, pose);
  float w = (float)sprite->w * scale, h = (float)sprite->h * scale;
  float ax = (float)sprite->anchor_x * scale;
  float left = flip ? x - (w - ax) : x - ax;
  draw_t options = {flip, alpha, 0.0f, 0, 0, 0};
  gfx_draw(f->gfx, sprite->texture, NULL, rect(left - f->cam_x, y - h, w, h),
           &options);
}

/* Centre-anchored sprite, any size and angle. */
static void draw_centred(const frame_t* f, pose_t pose, float cx, float cy,
                         float sx, float sy, float angle, bool flip,
                         float alpha) {
  const sprite_t* sprite = look_sprite(f->look, pose);
  float w = (float)sprite->w * sx, h = (float)sprite->h * sy;
  draw_t options = {flip, alpha, angle, 0, 0, 0};
  gfx_draw(f->gfx, sprite->texture, NULL,
           rect(cx - w * 0.5f - f->cam_x, cy - h * 0.5f, w, h), &options);
}

static void draw_texture_centred(const frame_t* f, texture_t texture, float cx,
                                 float cy, float w, float h, float alpha) {
  draw_t options = {false, alpha, 0.0f, 0, 0, 0};
  gfx_draw(f->gfx, texture, NULL,
           rect(cx - w * 0.5f - f->cam_x, cy - h * 0.5f, w, h), &options);
}

static void draw_backdrop(const frame_t* f) {
  gfx_draw(f->gfx, f->look->sky, NULL, rect(0, 0, f->view_w, GFX_VIEW_HEIGHT), NULL);

  /* Clouds drift slowly with the camera. */
  float period = 3200.0f;
  for (int i = 0; i < 9; i++) {
    float base = (float)i * 420.0f + (float)((i * 97) % 130);
    float x = fmodf(base - f->cam_x * 0.12f - f->world->time * 6.0f, period);
    if (x < -300.0f) x += period;
    texture_t cloud = f->look->clouds[i % CLOUD_VARIANTS];
    float scale = 4.0f + (float)(i % 3);
    float y = 36.0f + (float)((i * 61) % 150);
    gfx_draw(f->gfx, cloud, NULL,
             rect(x, y, (float)cloud.w * scale, (float)cloud.h * scale), NULL);
  }

  static const float factor[MOUNTAIN_LAYERS] = {0.08f, 0.2f, 0.38f};
  float span = (float)(LAYER_W * SCENERY_PIXEL);
  for (int layer = 0; layer < MOUNTAIN_LAYERS; layer++) {
    float offset = fmodf(f->cam_x * factor[layer], span);
    for (float x = -offset; x < f->view_w; x += span)
      gfx_draw(f->gfx, f->look->layers[layer], NULL,
               rect(x, 0, span, (float)(LAYER_H * SCENERY_PIXEL)), NULL);
  }
}

static void draw_decor(const frame_t* f) {
  const level_t* level = &f->world->level;
  for (int i = 0; i < level->decor_count; i++) {
    const decor_t* d = &level->decor[i];
    texture_t tree = f->look->trees[d->kind];
    float scale = d->kind == DECOR_BUSH ? 2.4f : 2.6f;
    float w = (float)tree.w * scale, h = (float)tree.h * scale;
    if (d->x + w < f->cam_x || d->x - w > f->cam_x + f->view_w) continue;
    gfx_draw(f->gfx, tree, NULL, rect(d->x - w * 0.5f - f->cam_x, d->y - h + 4.0f, w, h),
             NULL);
  }
  float cw = 64.0f * 3.6f;
  float cx = level->castle_x;
  if (cx + cw > f->cam_x && cx - cw < f->cam_x + f->view_w)
    gfx_draw(f->gfx, f->look->castle, NULL,
             rect(cx - cw * 0.5f - f->cam_x, (float)(GROUND_ROW * TILE) - cw + 2.0f, cw, cw),
             NULL);
}

static float bump_offset(const world_t* world, int col, int row) {
  for (int i = 0; i < MAX_BUMPS; i++) {
    const bump_t* b = &world->bumps[i];
    if (b->age > 0.0f && b->col == col && b->row == row)
      return -sinf(b->age / 0.18f * PI) * 9.0f;
  }
  return 0.0f;
}

static void draw_tile_sprite(const frame_t* f, strip_id_t strip, float x, float y) {
  const sprite_t* s = look_sprite(f->look, (pose_t){strip, 0});
  /* A hair larger than the tile hides seams between neighbours. */
  gfx_draw(f->gfx, s->texture, NULL,
           rect(x - f->cam_x - 0.4f, y - 0.4f, TILE + 0.8f, TILE + 0.8f), NULL);
}

static void draw_tiles(const frame_t* f) {
  const level_t* level = &f->world->level;
  int first = (int)floorf(f->cam_x / TILE) - 1;
  int last = (int)ceilf((f->cam_x + f->view_w) / TILE) + 1;
  if (first < 0) first = 0;
  if (last >= LEVEL_COLS) last = LEVEL_COLS - 1;
  for (int col = first; col <= last; col++) {
    for (int row = 0; row < LEVEL_ROWS; row++) {
      tile_t tile = (tile_t)level->tiles[row][col];
      if (tile == TILE_EMPTY) continue;
      float x = (float)(col * TILE);
      float y = (float)(row * TILE) + bump_offset(f->world, col, row);
      switch (tile) {
        case TILE_GRASS: {
          const sprite_t* grass = look_sprite(f->look, (pose_t){STRIP_GROUND_GRASS, 0});
          const sprite_t* dirt = look_sprite(f->look, (pose_t){STRIP_GROUND_DIRT, 0});
          rect_t gsrc = {(float)((col % 3) * 40), 0, 40, (float)grass->h};
          rect_t dsrc = {(float)((col % 3) * 40), 0, 40, (float)dirt->h};
          gfx_draw(f->gfx, dirt->texture, &dsrc,
                   rect(x - f->cam_x - 0.4f, y + 10.0f, TILE + 0.8f, 22.4f), NULL);
          gfx_draw(f->gfx, grass->texture, &gsrc,
                   rect(x - f->cam_x - 0.4f, y - 1.0f, TILE + 0.8f, 16.0f), NULL);
          break;
        }
        case TILE_ROCK: draw_tile_sprite(f, STRIP_GROUND_ROCK, x, y); break;
        case TILE_BRICK: draw_tile_sprite(f, STRIP_TILE_BRICK, x, y); break;
        case TILE_QBLOCK: draw_tile_sprite(f, STRIP_TILE_Q, x, y); break;
        case TILE_USED: draw_tile_sprite(f, STRIP_TILE_SOLID, x, y); break;
        case TILE_GOLD: draw_tile_sprite(f, STRIP_TILE_GOLD, x, y); break;
        case TILE_STONE: draw_tile_sprite(f, STRIP_TILE_STONE, x, y); break;
        case TILE_PIPE_TOP_L:
        case TILE_PIPE_TOP_R:
        case TILE_PIPE_BODY_L:
        case TILE_PIPE_BODY_R: {
          bool top = tile == TILE_PIPE_TOP_L || tile == TILE_PIPE_TOP_R;
          bool right = tile == TILE_PIPE_TOP_R || tile == TILE_PIPE_BODY_R;
          const sprite_t* s = look_sprite(f->look, (pose_t){top ? STRIP_PIPE_CAP : STRIP_PIPE_BODY, 0});
          rect_t src = {right ? (float)(s->w / 2) : 0.0f, 0, (float)(s->w / 2), (float)s->h};
          gfx_draw(f->gfx, s->texture, &src,
                   rect(x - f->cam_x - 0.4f, y - 0.4f, TILE + 0.8f, TILE + 0.8f), NULL);
          break;
        }
        default: break;
      }
    }
  }
}

static void draw_items(const frame_t* f) {
  const level_t* level = &f->world->level;
  float t = f->world->time;
  for (int i = 0; i < level->item_count; i++) {
    if (f->world->item_taken[i]) continue;
    const placement_t* item = &level->items[i];
    if (item->x < f->cam_x - 40.0f || item->x > f->cam_x + f->view_w + 40.0f) continue;
    float bob = sinf(t * 3.0f + item->x * 0.05f) * 3.0f;
    switch (item->kind) {
      case ITEM_COIN: {
        float spin = fabsf(cosf(t * 4.0f + item->x * 0.1f));
        draw_centred(f, (pose_t){STRIP_COIN, 0}, item->x, item->y + bob,
                     0.62f * (0.25f + 0.75f * spin), 0.62f, 0.0f, false, 1.0f);
        break;
      }
      case ITEM_POTION: {
        draw_texture_centred(f, f->look->glow, item->x, item->y + bob, 64.0f + sinf(t * 5.0f) * 8.0f,
                             64.0f + sinf(t * 5.0f) * 8.0f, 0.7f);
        draw_centred(f, (pose_t){STRIP_BOTTLE, 0}, item->x, item->y + bob, 0.3f, 0.3f, 0.0f, false, 1.0f);
        break;
      }
      case ITEM_PISTOL:
        draw_centred(f, (pose_t){STRIP_PISTOL, 0}, item->x, item->y + bob, 0.4f, 0.4f, 0.0f, false, 1.0f);
        break;
      case ITEM_MUSHROOM:
        draw_centred(f, (pose_t){STRIP_MUSHROOM, 0}, item->x, item->y + bob, 0.4f, 0.4f, 0.0f, false, 1.0f);
        break;
      default: break;
    }
  }
  for (int i = 0; i < MAX_PICKUPS; i++) {
    const pickup_t* p = &f->world->pickups[i];
    if (!p->alive) continue;
    strip_id_t strip = p->kind == ITEM_POTION ? STRIP_BOTTLE
                       : p->kind == ITEM_PISTOL ? STRIP_PISTOL : STRIP_MUSHROOM;
    float scale = p->kind == ITEM_POTION ? 0.3f : 0.4f;
    if (p->kind == ITEM_POTION)
      draw_texture_centred(f, f->look->glow, p->x, p->y, 64.0f, 64.0f, 0.7f);
    draw_centred(f, (pose_t){strip, 0}, p->x, p->y, scale, scale, 0.0f, false, 1.0f);
  }
}

static void draw_enemies(const frame_t* f) {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    const enemy_t* e = &f->world->enemies[i];
    if (!e->alive) continue;
    if (e->x < f->cam_x - 120.0f || e->x > f->cam_x + f->view_w + 120.0f) continue;
    if (e->kind == ENEMY_ZOMBIE) {
      draw_sprite(f, pose_zombie(e, &f->world->player), e->x, e->y, ZOMBIE_SCALE,
                  e->facing < 0, 1.0f);
      /* "I DON'T TEST!" floats and sways above the head. */
      float t = f->world->time * 2.6f + e->phase_seed;
      float sway = sinf(t * 0.7f) * 5.0f, bob = sinf(t) * 4.0f;
      float pulse = 1.0f + sinf(t * 2.3f) * 0.05f;
      draw_centred(f, (pose_t){STRIP_BUBBLE, 0}, e->x + 12.0f * (float)(-e->facing) * 0.0f + sway,
                   e->y - 56.0f - 38.0f + bob, 0.78f * pulse, 0.78f * pulse,
                   sinf(t * 0.9f) * 4.0f, false, 1.0f);
    } else {
      static const float scale[BUG_VARIANTS] = {0.4f, 0.42f, 0.42f, 0.4f, 0.4f};
      float wiggle = sinf(e->anim_time * 18.0f) * 7.0f;
      bool flier = e->variant == BUG_FLY || e->variant == BUG_BEE;
      float flap = flier ? 1.0f + sinf(e->anim_time * 40.0f) * 0.1f : 1.0f;
      float s = scale[e->variant];
      /* The bugs are drawn from above, head up: turn them to face the way
       * they go. */
      draw_centred(f, pose_bug(e), e->x, e->y - e->h * 0.5f, s * flap, s,
                   (e->facing > 0 ? 90.0f : -90.0f) + wiggle, false, 1.0f);
    }
  }
}

static void draw_player(const frame_t* f) {
  const player_t* p = &f->world->player;
  if (p->spell_time > 0.0f) {
    float fade = fminf(1.0f, p->spell_time / 3.0f);
    float pulse = 1.0f + sinf(f->world->time * 9.0f) * 0.08f;
    draw_texture_centred(f, f->look->glow, p->x, p->y - 28.0f, 150.0f * pulse,
                         170.0f * pulse, 0.85f * fade);
  }
  float alpha = 1.0f;
  if (p->alive && p->invulnerable > 0.0f && fmodf(f->world->time, 0.12f) < 0.06f)
    alpha = 0.35f;
  /* A bounce with every footfall and a lean into the stride, on top of the
   * frames. */
  float bounce = 0.0f, lean = 0.0f;
  if (p->alive && p->grounded && fabsf(p->vx) > 12.0f && p->hurt_timer <= 0.0f) {
    bounce = -fabsf(sinf(p->walk_phase * PI / 4.0f)) * 3.0f;
    lean = (fabsf(p->vx) > 230.0f ? 5.0f : 3.0f) * (float)p->facing;
  }
  const sprite_t* sprite = look_sprite(f->look, pose_player(p));
  float w = (float)sprite->w * FABIO_SCALE, h = (float)sprite->h * FABIO_SCALE;
  float ax = (float)sprite->anchor_x * FABIO_SCALE;
  bool flip = p->facing < 0;
  float left = flip ? p->x - (w - ax) : p->x - ax;
  draw_t options = {flip, alpha, lean, 0, 0, 0};
  gfx_draw(f->gfx, sprite->texture, NULL,
           rect(left - f->cam_x, p->y - h + bounce, w, h), &options);
}

static void draw_effects(const frame_t* f) {
  for (int i = 0; i < MAX_EFFECTS; i++) {
    const effect_t* fx = &f->world->effects[i];
    if (!fx->alive) continue;
    float life = fx->age / fx->duration;
    float scale, lift;
    switch (fx->kind) {
      case FX_ZOMBIE_BOOM: scale = 0.7f + life * 0.5f; lift = 40.0f; break;
      case FX_ZOMBIE_GREEN: scale = 0.8f + life * 0.4f; lift = 40.0f; break;
      case FX_BUG_BOOM: scale = 0.45f + life * 0.4f; lift = 14.0f; break;
      case FX_BUG_BLUE: scale = 0.4f + life * 0.35f; lift = 14.0f; break;
      case FX_MUZZLE: scale = 0.4f; lift = 0.0f; break;
      default: scale = 0.6f; lift = 20.0f; break;
    }
    float alpha = fx->kind == FX_MUZZLE ? 1.0f : 1.0f - life * life;
    float x = fx->x, y = fx->y - lift;
    if (fx->kind == FX_MUZZLE) x += (float)fx->facing * 18.0f;
    draw_centred(f, pose_effect(fx->kind), x, y, scale, scale, 0.0f,
                 fx->kind == FX_MUZZLE && fx->facing < 0, alpha);
  }
  for (int i = 0; i < MAX_BULLETS; i++) {
    const bullet_t* b = &f->world->bullets[i];
    if (b->alive)
      draw_centred(f, (pose_t){STRIP_FX_BULLET, 0}, b->x, b->y, 0.3f, 0.3f, 0.0f,
                   b->vx < 0.0f, 1.0f);
  }
  for (int i = 0; i < MAX_RAYS; i++) {
    const ray_t* r = &f->world->rays[i];
    if (!r->alive) continue;
    const sprite_t* s = look_sprite(f->look, (pose_t){STRIP_FX_RAY, 0});
    float length = r->x1 - r->x0;
    float jitter = sinf(r->age * 120.0f) * 3.0f;
    float left = length >= 0.0f ? r->x0 : r->x1;
    draw_t options = {r->facing < 0, 1.0f - r->age / 0.22f, 0.0f, 0, 0, 0};
    gfx_draw(f->gfx, s->texture, NULL,
             rect(left - f->cam_x, r->y - 15.0f + jitter, fabsf(length), 30.0f), &options);
    draw_t halo = {false, 0.5f * (1.0f - r->age / 0.22f), 0.0f, 0, 0, 0};
    gfx_draw(f->gfx, f->look->glow, NULL,
             rect(left - f->cam_x, r->y - 24.0f, fabsf(length), 48.0f), &halo);
  }
  for (int i = 0; i < MAX_PARTICLES; i++) {
    const particle_t* p = &f->world->particles[i];
    if (!p->alive) continue;
    float a = 1.0f - p->age / p->life;
    gfx_rect(f->gfx, rect(p->x - f->cam_x, p->y, p->size, p->size), p->r, p->g, p->b,
             (uint8_t)(a * 255.0f));
  }
}

/* ---- text and the heads-up display ---- */

static void shadow_text(gfx_t* gfx, float x, float y, const char* text, float scale,
                        uint8_t r, uint8_t g, uint8_t b) {
  gfx_text(gfx, x + scale, y + scale, text, scale, 20, 10, 40);
  gfx_text(gfx, x, y, text, scale, r, g, b);
}

static void centred_text(gfx_t* gfx, float cx, float y, const char* text, float scale,
                         uint8_t r, uint8_t g, uint8_t b) {
  shadow_text(gfx, cx - gfx_text_width(text, scale) * 0.5f, y, text, scale, r, g, b);
}

static void draw_heart(gfx_t* gfx, float x, float y, float px, bool full) {
  static const char* kHeart[6] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
  for (int row = 0; row < 6; row++)
    for (int col = 0; col < 7; col++)
      if (kHeart[row][col] == '#') {
        if (full) {
          bool shine = row == 1 && col == 1;
          gfx_rect(gfx, rect(x + col * px, y + row * px, px, px), shine ? 255 : 226,
                   shine ? 200 : 40, shine ? 200 : 56, 255);
        } else {
          gfx_rect(gfx, rect(x + col * px, y + row * px, px, px), 60, 40, 70, 200);
        }
      }
}

static void draw_hud(const frame_t* f) {
  gfx_t* gfx = f->gfx;
  const world_t* w = f->world;
  const player_t* p = &w->player;
  gfx_rect(gfx, rect(0, 0, f->view_w, 66.0f), 10, 10, 30, 140);

  for (int i = 0; i < PLAYER_HP; i++) draw_heart(gfx, 14.0f + (float)i * 30.0f, 10.0f, 3.5f, i < p->hp);

  char text[96];
  snprintf(text, sizeof(text), "SCORE %06d", w->score);
  shadow_text(gfx, 14.0f, 40.0f, text, 3.0f, 255, 240, 120);

  float x = 250.0f;
  draw_t none = {false, 1.0f, 0.0f, 0, 0, 0};
  const sprite_t* coin = look_sprite(f->look, (pose_t){STRIP_COIN, 0});
  gfx_draw(gfx, coin->texture, NULL, rect(x, 8.0f, 22.0f, 26.0f), &none);
  snprintf(text, sizeof(text), "X%02d", w->coins);
  shadow_text(gfx, x + 28.0f, 12.0f, text, 3.0f, 255, 255, 255);

  x += 120.0f;
  const sprite_t* bottle = look_sprite(f->look, (pose_t){STRIP_BOTTLE, 0});
  gfx_draw(gfx, bottle->texture, NULL, rect(x, 6.0f, 20.0f, 40.0f), &none);
  snprintf(text, sizeof(text), "X%d", p->potions);
  shadow_text(gfx, x + 26.0f, 12.0f, text, 3.0f, 140, 255, 140);
  shadow_text(gfx, x - 2.0f, 46.0f, "E TO DRINK", 1.6f, 200, 255, 200);

  x += 120.0f;
  if (p->has_gun) {
    const sprite_t* gun = look_sprite(f->look, (pose_t){STRIP_PISTOL, 0});
    gfx_draw(gfx, gun->texture, NULL, rect(x, 10.0f, 42.0f, 32.0f), &none);
    shadow_text(gfx, x - 2.0f, 46.0f, "J TO SHOOT", 1.6f, 220, 220, 220);
  } else {
    shadow_text(gfx, x, 14.0f, "NO GUN", 2.4f, 150, 150, 170);
  }

  if (p->spell_time > 0.0f) {
    float bar_w = 220.0f, bar_x = f->view_w - bar_w - 18.0f;
    bool blink = p->spell_time < 3.0f && fmodf(w->time, 0.3f) < 0.15f;
    shadow_text(gfx, bar_x, 8.0f, "TDD SPELL", 2.4f, 130, 220, 255);
    gfx_rect(gfx, rect(bar_x - 2.0f, 30.0f, bar_w + 4.0f, 20.0f), 20, 20, 50, 255);
    if (!blink)
      gfx_rect(gfx, rect(bar_x, 32.0f, bar_w * (p->spell_time / POTION_SECONDS), 16.0f),
               90, 200, 255, 255);
  }

  /* Progress to the castle. */
  float track_w = fminf(360.0f, f->view_w * 0.3f);
  float track_x = (f->view_w - track_w) * 0.5f - 60.0f;
  if (track_x > 780.0f) {
    gfx_rect(gfx, rect(track_x, 56.0f, track_w, 4.0f), 255, 255, 255, 160);
    float at = track_x + track_w * world_progress(w);
    gfx_rect(gfx, rect(at - 4.0f, 48.0f, 8.0f, 16.0f), 255, 60, 60, 255);
    gfx_rect(gfx, rect(track_x + track_w, 44.0f, 6.0f, 20.0f), 250, 210, 60, 255);
  }
}

static void draw_banner(const frame_t* f, const char* title, const char* subtitle,
                        uint8_t r, uint8_t g, uint8_t b) {
  gfx_t* gfx = f->gfx;
  gfx_rect(gfx, rect(0, 0, f->view_w, GFX_VIEW_HEIGHT), 10, 0, 20, 150);
  float cx = f->view_w * 0.5f;
  centred_text(gfx, cx, 170.0f, title, 9.0f, r, g, b);
  char text[96];
  snprintf(text, sizeof(text), "SCORE %06d   ZOMBIES %d   BUGS %d", f->world->score,
           f->world->zombies_killed, f->world->bugs_killed);
  centred_text(gfx, cx, 270.0f, text, 3.0f, 255, 255, 255);
  if (fmodf(f->world->state_time, 1.0f) < 0.65f)
    centred_text(gfx, cx, 350.0f, subtitle, 4.0f, 255, 240, 120);
}

void render_world(gfx_t* gfx, const look_t* look, const world_t* world) {
  frame_t f = {gfx, look, world, 0.0f, gfx_view_width(gfx)};
  /* Snap the camera to whole screen pixels so tiles never shimmer. */
  float scale = gfx_scale(gfx);
  f.cam_x = floorf(world->camera_x * scale) / scale;
  draw_backdrop(&f);
  draw_decor(&f);
  draw_tiles(&f);
  draw_items(&f);
  draw_enemies(&f);
  draw_player(&f);
  draw_effects(&f);
  draw_hud(&f);
  if (world->state == STATE_GAME_OVER)
    draw_banner(&f, "GAME OVER", "PRESS SPACE TO TRY AGAIN", 255, 70, 70);
  else if (world->state == STATE_WON)
    draw_banner(&f, "YOU DID IT!", "PRESS SPACE TO PLAY AGAIN", 120, 255, 140);
}

void render_pause(gfx_t* gfx) {
  float w = gfx_view_width(gfx);
  gfx_rect(gfx, rect(0, 0, w, GFX_VIEW_HEIGHT), 10, 0, 20, 140);
  centred_text(gfx, w * 0.5f, 220.0f, "PAUSED", 8.0f, 255, 255, 255);
}

void render_title(gfx_t* gfx, const look_t* look, float time) {
  float w = gfx_view_width(gfx);
  /* A scrolling red and green check, like the border of the box. */
  float cell = 64.0f, shift = fmodf(time * 20.0f, cell * 2.0f);
  gfx_rect(gfx, rect(0, 0, w, GFX_VIEW_HEIGHT), 120, 24, 30, 255);
  for (float y = -cell * 2.0f; y < GFX_VIEW_HEIGHT; y += cell)
    for (float x = -cell * 2.0f; x < w + cell; x += cell) {
      int cx = (int)floorf((x + shift) / cell), cy = (int)floorf((y + shift) / cell);
      if (((cx + cy) & 1) == 0)
        gfx_rect(gfx, rect(x + fmodf(shift, cell), y + fmodf(shift, cell), cell, cell), 26,
                 110, 52, 255);
    }
  gfx_rect(gfx, rect(0, 0, w, GFX_VIEW_HEIGHT), 0, 0, 20, 90);

  float size = 430.0f;
  float bob = sinf(time * 2.0f) * 4.0f;
  if (look->cover.handle) {
    gfx_rect(gfx, rect(w * 0.5f - size * 0.5f - 6.0f, 10.0f + bob - 6.0f, size + 12.0f, size + 12.0f),
             250, 220, 90, 255);
    gfx_draw(gfx, look->cover, NULL, rect(w * 0.5f - size * 0.5f, 10.0f + bob, size, size), NULL);
  }
  float cx = w * 0.5f;
  centred_text(gfx, cx, 456.0f, "SUPER FABIO BROS", 4.0f, 255, 220, 90);
  if (fmodf(time, 1.0f) < 0.65f)
    centred_text(gfx, cx, 494.0f, "PRESS SPACE TO START", 3.2f, 255, 255, 255);
  centred_text(gfx, cx, 528.0f, "DEDICATED TO MY FRIEND FABIO", 1.8f, 190, 230, 255);

  if (w > 1150.0f) {
    float x = w * 0.5f + size * 0.5f + 40.0f;
    static const char* kLines[] = {"ARROWS OR WASD  MOVE", "SPACE  JUMP", "SHIFT  RUN",
                                   "J  SHOOT OR CAST", "K  PICKAXE", "E  DRINK TDD POTION",
                                   "P  PAUSE", "M  MUSIC", "F11  FULL SCREEN"};
    shadow_text(gfx, x, 90.0f, "CONTROLS", 3.0f, 255, 220, 90);
    for (int i = 0; i < 9; i++) shadow_text(gfx, x, 130.0f + (float)i * 26.0f, kLines[i], 2.0f, 255, 255, 255);
    float lx = w * 0.5f - size * 0.5f - 40.0f;
    static const char* kTips[] = {"FIND THE PISTOL", "BREAK QUESTION BLOCKS", "DRINK THE TDD POTION",
                                  "ZOMBIES DON'T TEST", "REACH THE CASTLE"};
    float widest = 0.0f;
    for (int i = 0; i < 5; i++) widest = fmaxf(widest, gfx_text_width(kTips[i], 2.0f));
    shadow_text(gfx, lx - widest, 90.0f, "MISSION", 3.0f, 255, 220, 90);
    for (int i = 0; i < 5; i++)
      shadow_text(gfx, lx - widest, 130.0f + (float)i * 26.0f, kTips[i], 2.0f, 255, 255, 255);
  }
}

void render_caption(gfx_t* gfx, const char* text) {
  float w = gfx_view_width(gfx);
  gfx_rect(gfx, rect(0, GFX_VIEW_HEIGHT - 52.0f, w, 52.0f), 10, 0, 20, 170);
  centred_text(gfx, w * 0.5f, GFX_VIEW_HEIGHT - 40.0f, text, 3.4f, 255, 240, 120);
}

void render_countdown(gfx_t* gfx, float seconds_left) {
  float w = gfx_view_width(gfx);
  gfx_rect(gfx, rect(0, 0, w, GFX_VIEW_HEIGHT), 16, 10, 40, 255);
  char text[16];
  snprintf(text, sizeof(text), "%d", (int)ceilf(seconds_left));
  centred_text(gfx, w * 0.5f, 140.0f, text, 24.0f, 255, 220, 90);
  centred_text(gfx, w * 0.5f, 340.0f, "STARTING SOON", 4.0f, 255, 255, 255);
  centred_text(gfx, w * 0.5f, 390.0f, "START YOUR SCREEN RECORDING", 2.4f, 190, 230, 255);
}
