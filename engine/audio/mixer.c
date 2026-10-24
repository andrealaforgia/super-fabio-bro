#include "mixer.h"

#include <stddef.h>
#include <string.h>

void mixer_init(mixer_t* mixer) { memset(mixer, 0, sizeof(*mixer)); }

void mixer_play(mixer_t* mixer, const sample_t* sample, float volume) {
  if (!sample || !sample->data || sample->length <= 0) return;
  voice_t* chosen = NULL;
  for (int i = 0; i < MIXER_VOICES; i++) {
    if (!mixer->voices[i].sample) {
      chosen = &mixer->voices[i];
      break;
    }
    if (!chosen || mixer->voices[i].position > chosen->position)
      chosen = &mixer->voices[i];
  }
  chosen->sample = sample;
  chosen->position = 0;
  chosen->volume = volume;
}

void mixer_render(mixer_t* mixer, int16_t* out, int frames) {
  for (int v = 0; v < MIXER_VOICES; v++) {
    voice_t* voice = &mixer->voices[v];
    if (!voice->sample) continue;
    int left = voice->sample->length - voice->position;
    int count = left < frames ? left : frames;
    for (int i = 0; i < count; i++) {
      int mixed = out[i] + (int)(voice->sample->data[voice->position + i] *
                                 voice->volume);
      out[i] = (int16_t)(mixed > 32767 ? 32767 : (mixed < -32768 ? -32768 : mixed));
    }
    voice->position += count;
    if (voice->position >= voice->sample->length) voice->sample = NULL;
  }
}

int mixer_active_voices(const mixer_t* mixer) {
  int count = 0;
  for (int i = 0; i < MIXER_VOICES; i++)
    if (mixer->voices[i].sample) count++;
  return count;
}
