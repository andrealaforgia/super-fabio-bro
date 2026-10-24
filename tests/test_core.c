#include <string.h>

#include "controls.h"
#include "mixer.h"
#include "rng.h"
#include "stepper.h"
#include "test_groups.h"
#include "test_support.h"

static void rng_is_deterministic_per_seed(void) {
  rng_t a, b, c;
  rng_seed(&a, 42);
  rng_seed(&b, 42);
  rng_seed(&c, 43);
  bool differs = false;
  for (int i = 0; i < 100; i++) {
    uint32_t x = rng_next(&a);
    CHECK(x == rng_next(&b));
    if (x != rng_next(&c)) differs = true;
  }
  CHECK(differs);
}

static void rng_stays_in_range(void) {
  rng_t rng;
  rng_seed(&rng, 7);
  for (int i = 0; i < 10000; i++) {
    float f = rng_float(&rng);
    CHECK(f >= 0.0f && f < 1.0f);
    int n = rng_range(&rng, -3, 4);
    CHECK(n >= -3 && n <= 4);
  }
  CHECK(rng_range(&rng, 5, 5) == 5);
}

static void rng_zero_seed_still_works(void) {
  rng_t rng;
  rng_seed(&rng, 0);
  CHECK(rng_next(&rng) != 0);
}

static void stepper_splits_a_long_frame(void) {
  double step;
  int n = stepper_plan(1.0 / 30.0, 1.0 / 120.0, 0.1, &step);
  CHECK(n == 4);
  CHECK_NEAR(step * n, 1.0 / 30.0, 1e-9);
  CHECK(step <= 1.0 / 120.0 + 1e-12);
}

static void stepper_caps_a_stalled_frame(void) {
  double step;
  int n = stepper_plan(5.0, 1.0 / 120.0, 0.1, &step);
  CHECK_NEAR(step * n, 0.1, 1e-9);
}

static void stepper_always_runs_one_step(void) {
  double step;
  CHECK(stepper_plan(0.0, 1.0 / 120.0, 0.1, &step) == 1);
  CHECK(stepper_plan(-1.0, 1.0 / 120.0, 0.1, &step) == 1);
  CHECK(step >= 0.0);
}

static void controls_report_edges_once(void) {
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  buttons_t down;
  memset(&down, 0, sizeof(down));
  down.jump = true;
  controls_update(&controls, &down);
  CHECK(controls.held.jump && controls.pressed.jump);
  controls_update(&controls, &down);
  CHECK(controls.held.jump && !controls.pressed.jump);
  buttons_t up;
  memset(&up, 0, sizeof(up));
  controls_update(&controls, &up);
  CHECK(!controls.held.jump && !controls.pressed.jump);
  controls_update(&controls, &down);
  CHECK(controls.pressed.jump);
}

static void music_and_sound_toggles_are_separate_edges(void) {
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  buttons_t keys;
  memset(&keys, 0, sizeof(keys));
  keys.mute = true;
  controls_update(&controls, &keys);
  CHECK(controls.pressed.mute && !controls.pressed.sound);
  keys.mute = false;
  keys.sound = true;
  controls_update(&controls, &keys);
  CHECK(controls.pressed.sound && !controls.pressed.mute);
  controls_update(&controls, &keys);
  CHECK(!controls.pressed.sound);
}

static void mixer_plays_a_sample_once_then_stops(void) {
  int16_t data[100];
  for (int i = 0; i < 100; i++) data[i] = 1000;
  sample_t sample = {data, 100};
  mixer_t mixer;
  mixer_init(&mixer);
  mixer_play(&mixer, &sample, 1.0f);
  CHECK(mixer_active_voices(&mixer) == 1);
  int16_t out[150];
  memset(out, 0, sizeof(out));
  mixer_render(&mixer, out, 150);
  CHECK(out[0] == 1000 && out[99] == 1000 && out[100] == 0);
  CHECK(mixer_active_voices(&mixer) == 0);
}

static void mixer_sums_voices_and_clips(void) {
  int16_t data[10];
  for (int i = 0; i < 10; i++) data[i] = 30000;
  sample_t sample = {data, 10};
  mixer_t mixer;
  mixer_init(&mixer);
  mixer_play(&mixer, &sample, 1.0f);
  mixer_play(&mixer, &sample, 1.0f);
  int16_t out[10] = {0};
  mixer_render(&mixer, out, 10);
  CHECK(out[0] == 32767);
}

static void mixer_steals_a_voice_when_full(void) {
  int16_t data[1000] = {0};
  sample_t sample = {data, 1000};
  mixer_t mixer;
  mixer_init(&mixer);
  for (int i = 0; i < MIXER_VOICES + 4; i++) mixer_play(&mixer, &sample, 1.0f);
  CHECK(mixer_active_voices(&mixer) == MIXER_VOICES);
}

void run_core_tests(void) {
  RUN(rng_is_deterministic_per_seed);
  RUN(rng_stays_in_range);
  RUN(rng_zero_seed_still_works);
  RUN(stepper_splits_a_long_frame);
  RUN(stepper_caps_a_stalled_frame);
  RUN(stepper_always_runs_one_step);
  RUN(controls_report_edges_once);
  RUN(music_and_sound_toggles_are_separate_edges);
  RUN(mixer_plays_a_sample_once_then_stops);
  RUN(mixer_sums_voices_and_clips);
  RUN(mixer_steals_a_voice_when_full);
}
