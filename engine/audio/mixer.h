/**
 * @file mixer.h
 * @brief A pure voice mixer: mono 16-bit samples played at once, summed and
 * clipped. No device in here, so tests can render it to memory.
 */
#ifndef ENGINE_AUDIO_MIXER_H_
#define ENGINE_AUDIO_MIXER_H_

#include <stdint.h>

#define MIXER_RATE 22050
#define MIXER_VOICES 16

typedef struct {
  const int16_t* data;
  int length;
} sample_t;

typedef struct {
  const sample_t* sample;
  int position;
  float volume;
} voice_t;

typedef struct {
  voice_t voices[MIXER_VOICES];
} mixer_t;

void mixer_init(mixer_t* mixer);
/* Starts a voice; steals the one furthest along when all are busy. */
void mixer_play(mixer_t* mixer, const sample_t* sample, float volume);
/* Adds `frames` mixed samples into `out` (it is not cleared). */
void mixer_render(mixer_t* mixer, int16_t* out, int frames);
int mixer_active_voices(const mixer_t* mixer);

#endif  // ENGINE_AUDIO_MIXER_H_
