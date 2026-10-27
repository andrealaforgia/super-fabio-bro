#include <stdlib.h>
#include <string.h>

#include "scenery.h"
#include "test_groups.h"
#include "test_support.h"

static int alpha(uint32_t pixel) { return (int)(pixel >> 24); }

static int opaque_count(const image_t* image) {
  int n = 0;
  for (int i = 0; i < image->w * image->h; i++) n += alpha(image->pixels[i]) == 255;
  return n;
}

static void the_sky_gets_paler_towards_the_horizon(void) {
  image_t sky;
  CHECK(scenery_sky(&sky, 64));
  CHECK(sky.w == 1 && sky.h == 64);
  int top = (int)(sky.pixels[0] & 255), bottom = (int)(sky.pixels[63] & 255);
  CHECK(bottom > top + 60);
  image_free(&sky);
}

static void every_layer_tiles_without_a_seam(void) {
  for (int layer = 0; layer < MOUNTAIN_LAYERS; layer++) {
    int worst = 0;
    for (int x = 0; x < LAYER_W; x++) {
      int step = abs(scenery_ridge(layer, x + 1) - scenery_ridge(layer, x));
      if (step > worst) worst = step;
    }
    CHECK(worst <= 6);
    CHECK(scenery_ridge(layer, -1) == scenery_ridge(layer, LAYER_W - 1));
    CHECK(scenery_ridge(layer, LAYER_W) == scenery_ridge(layer, 0));
  }
}

static void layers_stand_in_front_of_each_other_far_to_near(void) {
  double mean[MOUNTAIN_LAYERS];
  for (int layer = 0; layer < MOUNTAIN_LAYERS; layer++) {
    double sum = 0.0;
    for (int x = 0; x < LAYER_W; x++) sum += scenery_ridge(layer, x);
    mean[layer] = sum / LAYER_W;
    CHECK(mean[layer] > 5.0 && mean[layer] < LAYER_H - 10);
  }
  CHECK(mean[0] < mean[1] && mean[1] < mean[2]);
}

static void a_layer_is_sky_above_the_ridge_and_solid_below(void) {
  image_t layer;
  CHECK(scenery_layer(&layer, 0));
  for (int x = 0; x < LAYER_W; x += 17) {
    int top = scenery_ridge(0, x);
    CHECK(top >= 0 && top < LAYER_H);
    if (top > 0) CHECK(image_at(&layer, x, top - 1) == 0);
    CHECK(alpha(image_at(&layer, x, top)) == 255);
    CHECK(alpha(image_at(&layer, x, LAYER_H - 1)) == 255);
  }
  image_free(&layer);
}

static void the_far_range_has_snow_on_its_peaks(void) {
  image_t layer;
  CHECK(scenery_layer(&layer, 0));
  int white = 0;
  for (int i = 0; i < layer.w * layer.h; i++)
    white += (layer.pixels[i] & 255) > 240 && ((layer.pixels[i] >> 16) & 255) > 240;
  CHECK(white > 400);
  image_free(&layer);
}

static void clouds_trees_bushes_and_the_castle_have_a_body_and_clear_corners(void) {
  image_t images[6];
  CHECK(scenery_cloud(&images[0], 3));
  CHECK(scenery_tree(&images[1], DECOR_PINE, 1));
  CHECK(scenery_tree(&images[2], DECOR_OAK, 2));
  CHECK(scenery_tree(&images[3], DECOR_BUSH, 3));
  CHECK(scenery_castle(&images[4]));
  CHECK(scenery_glow(&images[5]));
  for (int i = 0; i < 6; i++) {
    CHECK(opaque_count(&images[i]) > 0 || i == 5);
    CHECK(alpha(image_at(&images[i], 0, 0)) == 0);
  }
  CHECK(opaque_count(&images[4]) > 1200);
  image_free(&images[0]);
  for (int i = 1; i < 6; i++) image_free(&images[i]);
}

static void the_castle_has_a_dark_gate_at_its_foot(void) {
  image_t castle;
  CHECK(scenery_castle(&castle));
  uint32_t door = image_at(&castle, 31, 60);
  CHECK(alpha(door) == 255);
  CHECK((door & 255) < 90);
  image_free(&castle);
}

static void the_glow_is_brightest_in_the_middle(void) {
  image_t glow;
  CHECK(scenery_glow(&glow));
  CHECK(alpha(image_at(&glow, 32, 32)) > 200);
  CHECK(alpha(image_at(&glow, 1, 1)) == 0);
  CHECK(alpha(image_at(&glow, 32, 32)) > alpha(image_at(&glow, 32, 56)));
  image_free(&glow);
}

static void tree_kinds_differ_in_shape(void) {
  image_t pine, bush;
  CHECK(scenery_tree(&pine, DECOR_PINE, 1));
  CHECK(scenery_tree(&bush, DECOR_BUSH, 1));
  CHECK(pine.h > bush.h * 2);
  image_free(&pine);
  image_free(&bush);
}

static void the_same_seed_always_gives_the_same_backdrop(void) {
  scenery_theme_t a = scenery_theme(12345u), b = scenery_theme(12345u);
  CHECK(memcmp(&a, &b, sizeof(a)) == 0);
}

static void different_runs_get_different_skies_and_starting_scrolls(void) {
  int distinct_skies = 0;
  bool seen[SCENERY_THEMES] = {false};
  float lowest = 1.0f, highest = 0.0f;
  for (uint32_t seed = 1700000000u; seed < 1700000200u; seed += 3u) {
    scenery_theme_t t = scenery_theme(seed);
    CHECK(t.kind >= 0 && t.kind < SCENERY_THEMES);
    if (!seen[t.kind]) distinct_skies++;
    seen[t.kind] = true;
    CHECK(t.scroll >= 0.0f && t.scroll < 1.0f);
    if (t.scroll < lowest) lowest = t.scroll;
    if (t.scroll > highest) highest = t.scroll;
  }
  CHECK(distinct_skies == SCENERY_THEMES);
  CHECK(highest - lowest > 0.6f);
}

static void a_theme_only_ever_tints_it_never_makes_the_backdrop_black(void) {
  for (uint32_t seed = 1; seed < 400u; seed++) {
    scenery_theme_t t = scenery_theme(seed);
    for (int c = 0; c < 3; c++) {
      CHECK(t.sky[c] >= 110 && t.cloud[c] >= 110);
      for (int layer = 0; layer < MOUNTAIN_LAYERS; layer++)
        CHECK(t.layer[layer][c] >= 110);
    }
  }
}

static void one_theme_is_plain_daylight(void) {
  bool found = false;
  for (uint32_t seed = 1; seed < 100u && !found; seed++) {
    scenery_theme_t t = scenery_theme(seed);
    found = t.sky[0] == 255 && t.sky[1] == 255 && t.sky[2] == 255;
  }
  CHECK(found);
}

void run_scenery_tests(void) {
  RUN(the_sky_gets_paler_towards_the_horizon);
  RUN(every_layer_tiles_without_a_seam);
  RUN(layers_stand_in_front_of_each_other_far_to_near);
  RUN(a_layer_is_sky_above_the_ridge_and_solid_below);
  RUN(the_far_range_has_snow_on_its_peaks);
  RUN(clouds_trees_bushes_and_the_castle_have_a_body_and_clear_corners);
  RUN(the_castle_has_a_dark_gate_at_its_foot);
  RUN(the_glow_is_brightest_in_the_middle);
  RUN(tree_kinds_differ_in_shape);
  RUN(the_same_seed_always_gives_the_same_backdrop);
  RUN(different_runs_get_different_skies_and_starting_scrolls);
  RUN(a_theme_only_ever_tints_it_never_makes_the_backdrop_black);
  RUN(one_theme_is_plain_daylight);
}
