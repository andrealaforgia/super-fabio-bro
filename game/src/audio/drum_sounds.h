/**
 * @file drum_sounds.h
 * @brief The clip for each of the six words Fabio's drums shout (see kDrumWords).
 */
#ifndef GAME_AUDIO_DRUM_SOUNDS_H_
#define GAME_AUDIO_DRUM_SOUNDS_H_

#include <stdint.h>

#include "world.h"

typedef struct {
  const int16_t* data; /* mono, at the mixer rate */
  int length;          /* in frames */
} drum_clip_t;

extern const drum_clip_t kDrumClips[DRUM_WORDS];

#endif  // GAME_AUDIO_DRUM_SOUNDS_H_
