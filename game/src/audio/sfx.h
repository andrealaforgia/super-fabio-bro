/**
 * @file sfx.h
 * @brief Every sound is synthesised at start-up: no audio files. Effects are
 * short mono clips for the mixer; the music is a looping chiptune rendered
 * on the fly.
 */
#ifndef GAME_AUDIO_SFX_H_
#define GAME_AUDIO_SFX_H_

#include "mixer.h"
#include "world.h"

typedef enum {
  SFX_JUMP,
  SFX_SHOOT,
  SFX_SPELL,
  SFX_MELEE,
  SFX_ZOMBIE_BOOM,
  SFX_BUG_BOOM,
  SFX_COIN,
  SFX_PICKUP,
  SFX_DRINK,
  SFX_HURT,
  SFX_BUMP,
  SFX_STOMP,
  SFX_BLOCK,
  SFX_GROAN,
  SFX_DEATH,
  SFX_WIN,
  SFX_HEAL,
  SFX_VOICE_FABIO, /* "It's me! Fabio!" */
  SFX_COUNT
} sfx_id_t;

/* Allocates the clips; release with sfx_free. */
bool sfx_build(sample_t clips[SFX_COUNT]);
void sfx_free(sample_t clips[SFX_COUNT]);

/* The clip a game event sounds like, and how loud. */
sfx_id_t sfx_for_event(event_kind_t kind, float* volume);

typedef struct {
  double time;
} music_t;

/* Adds the next `frames` samples of the tune into `out`. */
void music_mix(music_t* music, int16_t* out, int frames, float volume);
/* Length of one pass through the tune, in seconds. */
double music_loop_seconds(void);

#endif  // GAME_AUDIO_SFX_H_
