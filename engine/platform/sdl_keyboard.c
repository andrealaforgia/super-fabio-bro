#include "sdl_keyboard.h"

#include <SDL.h>

static bool down(const Uint8* keys, SDL_Scancode a, SDL_Scancode b) {
  return keys[a] || keys[b];
}

buttons_t keyboard_read(void) {
  const Uint8* keys = SDL_GetKeyboardState(NULL);
  buttons_t b;
  b.left = down(keys, SDL_SCANCODE_LEFT, SDL_SCANCODE_A);
  b.right = down(keys, SDL_SCANCODE_RIGHT, SDL_SCANCODE_D);
  b.up = down(keys, SDL_SCANCODE_UP, SDL_SCANCODE_W);
  b.down = keys[SDL_SCANCODE_DOWN];
  b.jump = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_Z] || b.up;
  b.run = down(keys, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT);
  b.shoot = keys[SDL_SCANCODE_J] || keys[SDL_SCANCODE_F] ||
            keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
  b.melee = down(keys, SDL_SCANCODE_K, SDL_SCANCODE_X);
  b.drink = down(keys, SDL_SCANCODE_E, SDL_SCANCODE_Q);
  b.pause = down(keys, SDL_SCANCODE_P, SDL_SCANCODE_ESCAPE);
  b.confirm = down(keys, SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER);
  b.restart = keys[SDL_SCANCODE_R];
  b.fullscreen = keys[SDL_SCANCODE_F11];
  b.mute = keys[SDL_SCANCODE_M];
  b.sound = keys[SDL_SCANCODE_S];
  return b;
}
