#include <stdlib.h>
#include <string.h>

#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"

static double peak(const sample_t* clip) {
  int p = 0;
  for (int i = 0; i < clip->length; i++) {
    int v = abs(clip->data[i]);
    if (v > p) p = v;
  }
  return p;
}

static void every_sound_effect_is_built_audible_and_short(void) {
  sample_t clips[SFX_COUNT];
  CHECK(sfx_build(clips));
  for (int i = 0; i < SFX_COUNT; i++) {
    CHECK(clips[i].data != NULL);
    CHECK(clips[i].length > MIXER_RATE / 10);
    CHECK(clips[i].length < MIXER_RATE * 2);
    CHECK(peak(&clips[i]) > 2000);
    /* The last sample is faded out, so clips end without a click. */
    CHECK(abs(clips[i].data[clips[i].length - 1]) < 400);
  }
  sfx_free(clips);
}

static void every_game_event_has_a_sound(void) {
  for (int kind = EVENT_JUMP; kind <= EVENT_HEAL; kind++) {
    float volume = 0.0f;
    sfx_id_t id = sfx_for_event((event_kind_t)kind, &volume);
    CHECK(id >= 0 && id < SFX_COUNT);
    CHECK(volume > 0.0f && volume <= 1.0f);
  }
}

static void explosions_are_louder_than_the_rest_of_the_noise(void) {
  float zombie = 0.0f, groan = 0.0f;
  sfx_for_event(EVENT_ZOMBIE_DIES, &zombie);
  sfx_for_event(EVENT_GROAN, &groan);
  CHECK(zombie > groan);
}

static void the_mixer_plays_a_built_clip_to_the_end(void) {
  sample_t clips[SFX_COUNT];
  CHECK(sfx_build(clips));
  mixer_t mixer;
  mixer_init(&mixer);
  mixer_play(&mixer, &clips[SFX_SHOOT], 1.0f);
  static int16_t out[MIXER_RATE];
  memset(out, 0, sizeof(out));
  mixer_render(&mixer, out, MIXER_RATE);
  CHECK(mixer_active_voices(&mixer) == 0);
  int loud = 0;
  for (int i = 0; i < MIXER_RATE; i++) loud += abs(out[i]) > 3000;
  CHECK(loud > 100);
  sfx_free(clips);
}

static void the_music_is_audible_and_loops_without_running_away(void) {
  music_t music = {0.0};
  static int16_t out[MIXER_RATE];
  memset(out, 0, sizeof(out));
  double energy = 0.0;
  for (int pass = 0; pass < 10; pass++) {
    music_mix(&music, out, MIXER_RATE, 0.5f);
    CHECK(music.time >= 0.0 && music.time < music_loop_seconds());
  }
  for (int i = 0; i < MIXER_RATE; i++) energy += fabs((double)out[i]);
  CHECK(energy / MIXER_RATE > 500.0);
  int clipped = 0;
  for (int i = 0; i < MIXER_RATE; i++) clipped += abs(out[i]) >= 32767;
  CHECK(clipped == 0);
}

static void music_adds_to_the_buffer_instead_of_replacing_it(void) {
  music_t music = {0.0};
  int16_t out[100];
  for (int i = 0; i < 100; i++) out[i] = 1000;
  music_mix(&music, out, 100, 0.0f);
  CHECK(out[0] == 1000 && out[99] == 1000);
}

static void the_spoken_lines_are_loud_and_long_enough_to_be_understood(void) {
  sample_t clips[SFX_COUNT];
  CHECK(sfx_build(clips));
  CHECK(peak(&clips[SFX_VOICE_FABIO]) > 28000);
  CHECK(clips[SFX_VOICE_FABIO].length > MIXER_RATE);
  CHECK(peak(&clips[SFX_GROAN]) > 25000);
  CHECK(clips[SFX_GROAN].length > MIXER_RATE * 3 / 2);
  /* The words are spread over the clip, not squeezed into its start. */
  const sample_t* voice = &clips[SFX_VOICE_FABIO];
  int late = 0;
  for (int i = voice->length * 2 / 3; i < voice->length; i++) late += abs(voice->data[i]) > 3000;
  CHECK(late > 500);
  sfx_free(clips);
}

void run_audio_tests(void) {
  RUN(the_spoken_lines_are_loud_and_long_enough_to_be_understood);
  RUN(every_sound_effect_is_built_audible_and_short);
  RUN(every_game_event_has_a_sound);
  RUN(explosions_are_louder_than_the_rest_of_the_noise);
  RUN(the_mixer_plays_a_built_clip_to_the_end);
  RUN(the_music_is_audible_and_loops_without_running_away);
  RUN(music_adds_to_the_buffer_instead_of_replacing_it);
}
