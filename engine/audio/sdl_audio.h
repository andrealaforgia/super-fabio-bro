/**
 * @file sdl_audio.h
 * @brief The SDL audio device: calls `fill` from the audio thread for mono
 * 16-bit samples at MIXER_RATE. Lock around anything `fill` also touches.
 */
#ifndef ENGINE_AUDIO_SDL_AUDIO_H_
#define ENGINE_AUDIO_SDL_AUDIO_H_

#include <stdbool.h>
#include <stdint.h>

typedef void (*audio_fill_fn)(void* user, int16_t* out, int frames);

/* Returns false (and the game stays silent) when no device opens. */
bool audio_open(audio_fill_fn fill, void* user);
void audio_lock(void);
void audio_unlock(void);
void audio_close(void);

#endif  // ENGINE_AUDIO_SDL_AUDIO_H_
