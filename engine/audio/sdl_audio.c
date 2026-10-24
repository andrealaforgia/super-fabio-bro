#include "sdl_audio.h"

#include <SDL.h>
#include <string.h>

#include "mixer.h"

static SDL_AudioDeviceID device;
static audio_fill_fn fill_function;
static void* fill_user;

static void callback(void* user, Uint8* stream, int length) {
  (void)user;
  int16_t* out = (int16_t*)stream;
  int frames = length / (int)sizeof(int16_t);
  memset(stream, 0, (size_t)length);
  if (fill_function) fill_function(fill_user, out, frames);
}

bool audio_open(audio_fill_fn fill, void* user) {
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
  fill_function = fill;
  fill_user = user;
  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = MIXER_RATE;
  want.format = AUDIO_S16SYS;
  want.channels = 1;
  want.samples = 1024;
  want.callback = callback;
  device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (!device) return false;
  SDL_PauseAudioDevice(device, 0);
  return true;
}

void audio_lock(void) {
  if (device) SDL_LockAudioDevice(device);
}

void audio_unlock(void) {
  if (device) SDL_UnlockAudioDevice(device);
}

void audio_close(void) {
  if (device) SDL_CloseAudioDevice(device);
  device = 0;
}
