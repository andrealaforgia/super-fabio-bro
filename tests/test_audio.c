#include <stdlib.h>
#include <string.h>

#include "sfx.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

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

/* How often the wave crosses zero in [from, to) seconds: a measure of pitch. */
static int crossings(const sample_t* clip, double from, double to) {
  int n = 0;
  for (int i = (int)(from * MIXER_RATE) + 1; i < (int)(to * MIXER_RATE) && i < clip->length; i++)
    n += (clip->data[i - 1] < 0) != (clip->data[i] < 0);
  return n;
}

static void an_item_coming_out_of_a_block_climbs_three_times_then_twinkles(void) {
  sample_t clips[SFX_COUNT];
  CHECK(sfx_build(clips));
  const sample_t* block = &clips[SFX_BLOCK];
  CHECK(block->length > MIXER_RATE / 2); /* more than a beep */
  /* Each climb rises: higher at its end than at its start. */
  for (int k = 0; k < 3; k++) {
    double start = k * 0.13;
    CHECK(crossings(block, start + 0.095, start + 0.125) > crossings(block, start + 0.005, start + 0.035));
  }
  /* And each starts higher than the one before. */
  CHECK(crossings(block, 0.135, 0.165) > crossings(block, 0.005, 0.035));
  CHECK(crossings(block, 0.265, 0.295) > crossings(block, 0.135, 0.165));
  /* The twinkle on top is the highest of all. */
  CHECK(crossings(block, 0.40, 0.43) > crossings(block, 0.355, 0.385));
  sfx_free(clips);
}

static void every_game_event_has_a_sound(void) {
  for (int kind = EVENT_JUMP; kind < EVENT_KIND_COUNT; kind++) {
    float volume = 0.0f;
    sfx_id_t id = sfx_for_event((event_kind_t)kind, &volume);
    CHECK(id >= 0 && id < SFX_COUNT);
    CHECK(volume > 0.0f && volume <= 1.0f);
  }
}

static void zombies_make_no_sound_of_their_own_and_the_gorilla_growls(void) {
  float volume;
  CHECK(sfx_for_event(EVENT_GORILLA_GROWLS, &volume) == SFX_GORILLA_GROWL);
  world_t* world = world_new(1, recording_events());
  spawn_zombie(world, world->player.x + 400.0f, world->player.y);
  CHECK(test_event_total == 0); /* turning up used to groan "I don't test" */
  world_free(world);
}

static void explosions_are_louder_than_the_rest_of_the_noise(void) {
  float zombie = 0.0f, bloop = 0.0f;
  sfx_for_event(EVENT_ZOMBIE_DIES, &zombie);
  sfx_for_event(EVENT_BUBBLE_SHOT, &bloop);
  CHECK(zombie > bloop);
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
  CHECK(peak(&clips[SFX_VOICE_SLOP]) > 25000);
  CHECK(clips[SFX_VOICE_SLOP].length > MIXER_RATE * 3 / 2);
  CHECK(peak(&clips[SFX_GORILLA_GROWL]) > 25000);
  CHECK(clips[SFX_GORILLA_GROWL].length > MIXER_RATE / 2);
  /* The words are spread over the clip, not squeezed into its start. */
  const sample_t* voice = &clips[SFX_VOICE_FABIO];
  int late = 0;
  for (int i = voice->length * 2 / 3; i < voice->length; i++) late += abs(voice->data[i]) > 3000;
  CHECK(late > 500);
  sfx_free(clips);
}

static void fabio_curses_when_he_is_hit_and_not_when_he_hits(void) {
  CHECK(event_makes_fabio_swear(EVENT_HURT));
  event_kind_t kills[] = {EVENT_ZOMBIE_DIES,  EVENT_BUG_DIES,     EVENT_BOOK_READER_DIES, EVENT_MONK_DIES,
                          EVENT_CHEF_DIES,    EVENT_GORILLA_DIES, EVENT_DEMON_DIES,       EVENT_MELEE,
                          EVENT_SHOOT,        EVENT_SPELL,        EVENT_SKEWER_THROWN,    EVENT_STOMP};
  for (size_t i = 0; i < sizeof(kills) / sizeof(kills[0]); i++) CHECK(!event_makes_fabio_swear(kills[i]));
  sample_t clips[SFX_COUNT];
  CHECK(sfx_build(clips));
  CHECK(peak(&clips[SFX_VOICE_CURSE]) > 28000);
  CHECK(clips[SFX_VOICE_CURSE].length > MIXER_RATE);
  sfx_free(clips);
}

void run_audio_tests(void) {
  RUN(fabio_curses_when_he_is_hit_and_not_when_he_hits);
  RUN(the_spoken_lines_are_loud_and_long_enough_to_be_understood);
  RUN(every_sound_effect_is_built_audible_and_short);
  RUN(every_game_event_has_a_sound);
  RUN(zombies_make_no_sound_of_their_own_and_the_gorilla_growls);
  RUN(an_item_coming_out_of_a_block_climbs_three_times_then_twinkles);
  RUN(explosions_are_louder_than_the_rest_of_the_noise);
  RUN(the_mixer_plays_a_built_clip_to_the_end);
  RUN(the_music_is_audible_and_loops_without_running_away);
  RUN(music_adds_to_the_buffer_instead_of_replacing_it);
}
