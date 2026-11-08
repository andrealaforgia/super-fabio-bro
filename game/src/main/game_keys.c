#include "game_keys.h"

#include "keyboard.h"

static const key_binding_t kBindings[] = {
    {BTN_LEFT, {KEY_LEFT, KEY_A, KEY_NONE, KEY_NONE}},
    {BTN_RIGHT, {KEY_RIGHT, KEY_D, KEY_NONE, KEY_NONE}},
    {BTN_UP, {KEY_UP, KEY_W, KEY_NONE, KEY_NONE}},
    {BTN_DOWN, {KEY_DOWN, KEY_NONE, KEY_NONE, KEY_NONE}},
    {BTN_JUMP, {KEY_SPACE, KEY_Z, KEY_UP, KEY_W}},
    {BTN_RUN, {KEY_LSHIFT, KEY_RSHIFT, KEY_NONE, KEY_NONE}},
    {BTN_SHOOT, {KEY_RETURN, KEY_KP_ENTER, KEY_LCTRL, KEY_RCTRL}},
    {BTN_MELEE, {KEY_K, KEY_X, KEY_NONE, KEY_NONE}},
    {BTN_DRINK, {KEY_E, KEY_Q, KEY_NONE, KEY_NONE}},
    {BTN_PAUSE, {KEY_P, KEY_ESCAPE, KEY_NONE, KEY_NONE}},
    {BTN_CONFIRM, {KEY_RETURN, KEY_KP_ENTER, KEY_NONE, KEY_NONE}},
    {BTN_RESTART, {KEY_R, KEY_NONE, KEY_NONE, KEY_NONE}},
    {BTN_FULLSCREEN, {KEY_F11, KEY_NONE, KEY_NONE, KEY_NONE}},
    {BTN_MUTE, {KEY_M, KEY_NONE, KEY_NONE, KEY_NONE}},
    {BTN_SOUND, {KEY_S, KEY_NONE, KEY_NONE, KEY_NONE}},
};

buttons_t read_buttons(void) {
  return keyboard_read(kBindings, sizeof(kBindings) / sizeof(kBindings[0]));
}
