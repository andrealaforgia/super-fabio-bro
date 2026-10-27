#include "scenery.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"

#define TAU 6.28318530718

bool image_create(image_t* image, int w, int h) {
  image->w = w;
  image->h = h;
  image->pixels = calloc((size_t)w * (size_t)h, sizeof(uint32_t));
  return image->pixels != NULL;
}

void image_free(image_t* image) {
  free(image->pixels);
  image->pixels = NULL;
}

uint32_t image_at(const image_t* image, int x, int y) {
  if (x < 0 || y < 0 || x >= image->w || y >= image->h) return 0;
  return image->pixels[y * image->w + x];
}

static void plot(image_t* image, int x, int y, uint32_t color) {
  if (x < 0 || y < 0 || x >= image->w || y >= image->h) return;
  image->pixels[y * image->w + x] = color;
}

static void fill_rect(image_t* image, int x, int y, int w, int h, uint32_t color) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) plot(image, x + i, y + j, color);
}

static uint32_t mix(uint32_t a, uint32_t b, float t) {
  int ar = a & 255, ag = (a >> 8) & 255, ab = (a >> 16) & 255;
  int br = b & 255, bg = (b >> 8) & 255, bb = (b >> 16) & 255;
  return PIXEL((int)(ar + (br - ar) * t), (int)(ag + (bg - ag) * t),
               (int)(ab + (bb - ab) * t), 255);
}

bool scenery_sky(image_t* image, int h) {
  if (!image_create(image, 1, h)) return false;
  uint32_t top = PIXEL(52, 120, 214, 255);
  uint32_t middle = PIXEL(96, 168, 236, 255);
  uint32_t horizon = PIXEL(190, 224, 246, 255);
  for (int y = 0; y < h; y++) {
    float t = (float)y / (float)(h - 1);
    image->pixels[y] = t < 0.6f ? mix(top, middle, t / 0.6f)
                                : mix(middle, horizon, (t - 0.6f) / 0.4f);
  }
  return true;
}

scenery_theme_t scenery_theme(uint32_t seed) {
  /* Seeds taken a second apart must not look alike: scramble them first. */
  uint32_t h = seed * 2654435761u;
  h ^= h >> 15;
  h *= 2246822519u;
  h ^= h >> 13;
  static const uint8_t tint[SCENERY_THEMES][5][3] = {
      /* sky, far, middle, near, clouds */
      {{255, 255, 255}, {255, 255, 255}, {255, 255, 255}, {255, 255, 255}, {255, 255, 255}},
      {{255, 170, 130}, {255, 190, 170}, {240, 175, 140}, {215, 150, 120}, {255, 190, 150}},
      {{120, 120, 200}, {150, 150, 215}, {130, 135, 190}, {115, 120, 170}, {170, 165, 215}},
      {{255, 205, 225}, {255, 220, 235}, {235, 205, 215}, {200, 195, 190}, {255, 225, 235}},
      {{190, 200, 215}, {200, 210, 225}, {185, 200, 205}, {170, 190, 175}, {205, 210, 220}}};
  scenery_theme_t theme;
  memset(&theme, 0, sizeof(theme));
  theme.kind = (int)((h >> 4) % SCENERY_THEMES);
  memcpy(theme.sky, tint[theme.kind][0], 3);
  for (int layer = 0; layer < MOUNTAIN_LAYERS; layer++)
    memcpy(theme.layer[layer], tint[theme.kind][1 + layer], 3);
  memcpy(theme.cloud, tint[theme.kind][4], 3);
  theme.scroll = (float)((h >> 12) & 0xffffu) / 65536.0f;
  return theme;
}

/* Ridge shape: a sum of whole-number cosines, so it is periodic by
 * construction, folded to make peaks. */
static double ridge_value(int layer, int x) {
  static const struct {
    double base, amplitude;
    int f[4];
    double phase[4];
    double weight[4];
  } shape[MOUNTAIN_LAYERS] = {
      {34.0, 46.0, {2, 5, 11, 23}, {0.4, 2.1, 4.0, 1.3}, {1.0, 0.55, 0.3, 0.14}},
      {64.0, 36.0, {3, 7, 15, 29}, {1.7, 0.2, 3.3, 5.0}, {1.0, 0.5, 0.28, 0.12}},
      {98.0, 14.0, {4, 9, 19, 37}, {3.1, 1.0, 0.5, 2.2}, {1.0, 0.5, 0.25, 0.1}}};
  double sum = 0.0, total = 0.0;
  for (int k = 0; k < 4; k++) {
    sum += shape[layer].weight[k] *
           cos(TAU * shape[layer].f[k] * x / LAYER_W + shape[layer].phase[k]);
    total += shape[layer].weight[k];
  }
  double n = sum / total;                   /* -1..1 */
  double peak = layer == 2 ? n : 1.0 - 2.0 * fabs(n); /* folded: sharp tops */
  return shape[layer].base + shape[layer].amplitude * (layer == 2 ? -n : -peak) * 0.5;
}

int scenery_ridge(int layer, int x) {
  int wrapped = ((x % LAYER_W) + LAYER_W) % LAYER_W;
  return (int)floor(ridge_value(layer, wrapped));
}

static uint32_t hash_noise(int x, int y, int salt) {
  uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u +
               (uint32_t)salt * 2147483647u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

static void draw_pine_silhouette(image_t* image, int cx, int base, int height,
                                 uint32_t dark, uint32_t light) {
  for (int row = 0; row < height; row++) {
    float t = (float)row / (float)height;
    int half = 1 + (int)(t * (float)height * 0.28f) + (row % 5 == 0 ? 1 : 0);
    for (int dx = -half; dx <= half; dx++) {
      int x = ((cx + dx) % LAYER_W + LAYER_W) % LAYER_W;
      plot(image, x, base - height + row, dx < 0 && dx > -half ? light : dark);
    }
  }
}

bool scenery_layer(image_t* image, int layer) {
  if (!image_create(image, LAYER_W, LAYER_H)) return false;
  static const uint32_t lit[MOUNTAIN_LAYERS] = {
      PIXEL(138, 168, 206, 255), PIXEL(92, 150, 148, 255),
      PIXEL(62, 128, 84, 255)};
  static const uint32_t shade[MOUNTAIN_LAYERS] = {
      PIXEL(96, 126, 170, 255), PIXEL(62, 112, 118, 255),
      PIXEL(42, 98, 66, 255)};
  static const uint32_t haze[MOUNTAIN_LAYERS] = {
      PIXEL(176, 204, 232, 255), PIXEL(120, 172, 170, 255),
      PIXEL(40, 92, 64, 255)};
  for (int x = 0; x < LAYER_W; x++) {
    int top = scenery_ridge(layer, x);
    int left = scenery_ridge(layer, x - 2), right = scenery_ridge(layer, x + 2);
    bool facing_light = right < left; /* rising to the right: lit from the left */
    for (int y = top; y < LAYER_H; y++) {
      float depth = (float)(y - top);
      uint32_t base = facing_light ? lit[layer] : shade[layer];
      /* Fade to haze at the bottom, in the valleys. */
      float fog = (float)(y - 50) / (float)(LAYER_H - 50);
      if (fog < 0.0f) fog = 0.0f;
      uint32_t color = mix(base, haze[layer], fog * 0.85f);
      /* Rock striations. */
      if (layer < 2 && ((hash_noise(x / 3, y / 2, layer) & 7) == 0) && depth > 3.0f)
        color = mix(color, shade[layer], 0.35f);
      /* Snow caps on the far range. */
      if (layer == 0 && y < 46 + (int)(hash_noise(x, 0, 9) & 3) &&
          depth < 14.0f + (float)((hash_noise(x / 2, y, 3) & 3)))
        color = facing_light ? PIXEL(250, 252, 255, 255) : PIXEL(200, 214, 236, 255);
      plot(image, x, y, color);
    }
  }
  if (layer == 2) {
    /* Pines along the near hills. */
    rng_t rng;
    rng_seed(&rng, 77);
    for (int x = 0; x < LAYER_W; x += rng_range(&rng, 5, 9)) {
      int top = scenery_ridge(2, x);
      int height = rng_range(&rng, 12, 24);
      draw_pine_silhouette(image, x, top + 4, height, PIXEL(34, 88, 58, 255),
                           PIXEL(54, 116, 72, 255));
    }
  }
  return true;
}

bool scenery_cloud(image_t* image, uint32_t seed) {
  if (!image_create(image, 56, 24)) return false;
  rng_t rng;
  rng_seed(&rng, seed);
  int blobs = rng_range(&rng, 4, 6);
  for (int b = 0; b < blobs; b++) {
    float cx = 12.0f + (float)b * (32.0f / (float)blobs) + rng_float(&rng) * 6.0f;
    float cy = 14.0f - rng_float(&rng) * 4.0f - (b % 2) * 2.0f;
    float r = 5.0f + rng_float(&rng) * 4.0f;
    for (int y = 0; y < 24; y++)
      for (int x = 0; x < 56; x++) {
        float dx = (float)x - cx, dy = ((float)y - cy) * 1.3f;
        if (dx * dx + dy * dy <= r * r)
          plot(image, x, y, y < (int)cy - 1 ? PIXEL(255, 255, 255, 255)
                                              : PIXEL(222, 236, 250, 255));
      }
  }
  for (int x = 4; x < 52; x++)
    if (image_at(image, x, 15) != 0 && image_at(image, x, 16) == 0)
      plot(image, x, 16, PIXEL(206, 224, 244, 255));
  return true;
}

static void disc(image_t* image, float cx, float cy, float r, uint32_t color) {
  for (int y = (int)(cy - r) - 1; y <= (int)(cy + r) + 1; y++)
    for (int x = (int)(cx - r) - 1; x <= (int)(cx + r) + 1; x++) {
      float dx = (float)x - cx, dy = (float)y - cy;
      if (dx * dx + dy * dy <= r * r) plot(image, x, y, color);
    }
}

static void outline(image_t* image, uint32_t color) {
  image_t copy;
  if (!image_create(&copy, image->w, image->h)) return;
  memcpy(copy.pixels, image->pixels, (size_t)image->w * (size_t)image->h * 4);
  for (int y = 0; y < image->h; y++)
    for (int x = 0; x < image->w; x++) {
      if (image_at(&copy, x, y) != 0) continue;
      if (image_at(&copy, x - 1, y) || image_at(&copy, x + 1, y) ||
          image_at(&copy, x, y - 1) || image_at(&copy, x, y + 1))
        plot(image, x, y, color);
    }
  image_free(&copy);
}

bool scenery_tree(image_t* image, decor_kind_t kind, uint32_t seed) {
  rng_t rng;
  rng_seed(&rng, seed);
  uint32_t trunk = PIXEL(104, 68, 38, 255), trunk_dark = PIXEL(76, 48, 28, 255);
  uint32_t leaf = PIXEL(52, 140, 66, 255), leaf_light = PIXEL(96, 188, 84, 255),
           leaf_dark = PIXEL(34, 98, 52, 255);
  if (kind == DECOR_PINE) {
    int w = 34, h = 58;
    if (!image_create(image, w, h)) return false;
    fill_rect(image, w / 2 - 2, h - 10, 4, 10, trunk);
    fill_rect(image, w / 2 + 1, h - 10, 1, 10, trunk_dark);
    int tiers = 3;
    for (int t = 0; t < tiers; t++) {
      int top = 2 + t * 14, height = 22 + t * 2;
      for (int row = 0; row < height && top + row < h - 8; row++) {
        int half = 2 + (row * (6 + t * 2)) / height;
        for (int dx = -half; dx <= half; dx++) {
          uint32_t c = dx < -half / 3 ? leaf_light : (dx > half / 2 ? leaf_dark : leaf);
          if (((row + dx) & 3) == 0 && dx < 0) c = leaf_light;
          plot(image, w / 2 + dx, top + row, c);
        }
      }
    }
  } else if (kind == DECOR_OAK) {
    int w = 46, h = 56;
    if (!image_create(image, w, h)) return false;
    fill_rect(image, w / 2 - 3, h - 22, 6, 22, trunk);
    fill_rect(image, w / 2 + 1, h - 22, 2, 22, trunk_dark);
    disc(image, 23, 20, 15, leaf);
    disc(image, 12, 26, 10, leaf);
    disc(image, 34, 26, 10, leaf);
    disc(image, 19, 14, 7, leaf_light);
    disc(image, 11, 22, 4, leaf_light);
    for (int y = 28; y < 38; y++)
      for (int x = 16; x < 42; x++)
        if (image_at(image, x, y) == leaf && ((x + y) % 3) == 0)
          plot(image, x, y, leaf_dark);
  } else {
    int w = 34, h = 18;
    if (!image_create(image, w, h)) return false;
    disc(image, 11, 11, 7, leaf);
    disc(image, 22, 11, 8, leaf);
    disc(image, 17, 8, 7, leaf_light);
    for (int x = 4; x < 30; x++)
      if (image_at(image, x, 17) == 0) plot(image, x, 17, leaf_dark);
    for (int i = 0; i < 4; i++)
      plot(image, 6 + rng_range(&rng, 0, 20), 6 + rng_range(&rng, 0, 8),
           PIXEL(240, 90, 90, 255));
  }
  outline(image, PIXEL(22, 58, 34, 255));
  return true;
}

bool scenery_castle(image_t* image) {
  int w = 64, h = 64;
  if (!image_create(image, w, h)) return false;
  uint32_t stone = PIXEL(176, 176, 186, 255), stone_dark = PIXEL(128, 128, 142, 255),
           stone_light = PIXEL(208, 208, 216, 255), roof = PIXEL(192, 56, 48, 255),
           roof_dark = PIXEL(140, 38, 36, 255), door = PIXEL(70, 40, 30, 255),
           window = PIXEL(36, 40, 70, 255), gold = PIXEL(250, 210, 60, 255);
  /* Keep and two towers. */
  fill_rect(image, 16, 28, 32, 36, stone);
  fill_rect(image, 4, 18, 14, 46, stone);
  fill_rect(image, 46, 18, 14, 46, stone);
  fill_rect(image, 22, 14, 20, 14, stone);
  /* Mortar lines. */
  for (int y = 16; y < 64; y += 4)
    for (int x = 4; x < 60; x++)
      if (image_at(image, x, y) == stone && ((x + (y / 4) * 3) % 8) != 0)
        plot(image, x, y, stone_dark);
  /* Shading on the right of each block. */
  for (int y = 14; y < 64; y++)
    for (int x = 4; x < 60; x++)
      if (image_at(image, x, y) != 0 && image_at(image, x + 1, y) == 0)
        plot(image, x, y, stone_dark);
  /* Battlements on the keep. */
  for (int x = 16; x < 48; x += 6) fill_rect(image, x, 24, 3, 4, stone_light);
  /* Roofs. */
  for (int t = 0; t < 3; t++) {
    int cx = t == 0 ? 11 : (t == 1 ? 53 : 32);
    int base = t == 2 ? 14 : 18, wide = t == 2 ? 12 : 9, tall = t == 2 ? 12 : 14;
    for (int row = 0; row < tall; row++) {
      int half = wide * (row + 1) / tall;
      for (int dx = -half; dx <= half; dx++)
        plot(image, cx + dx, base - tall + row + 1, dx > half / 3 ? roof_dark : roof);
    }
    fill_rect(image, cx, base - tall - 5, 1, 5, stone_dark);
    fill_rect(image, cx + 1, base - tall - 5, 4, 3, gold);
  }
  /* Gate and windows. */
  fill_rect(image, 26, 46, 12, 18, door);
  for (int dy = 0; dy < 6; dy++)
    fill_rect(image, 26 + dy / 2 + (dy < 2 ? 1 : 0), 42 + dy, 12 - dy - (dy < 2 ? 2 : 0), 1, door);
  fill_rect(image, 31, 50, 1, 14, PIXEL(40, 22, 16, 255));
  fill_rect(image, 8, 28, 4, 6, window);
  fill_rect(image, 50, 28, 4, 6, window);
  fill_rect(image, 29, 18, 4, 6, window);
  outline(image, PIXEL(60, 60, 76, 255));
  return true;
}

bool scenery_glow(image_t* image) {
  int size = 64;
  if (!image_create(image, size, size)) return false;
  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++) {
      float dx = (x - 31.5f) / 32.0f, dy = (y - 31.5f) / 32.0f;
      float d = sqrtf(dx * dx + dy * dy);
      float a = d >= 1.0f ? 0.0f : powf(1.0f - d, 1.8f);
      image->pixels[y * size + x] = PIXEL(255, 232, 120, (int)(a * 255.0f));
    }
  return true;
}
