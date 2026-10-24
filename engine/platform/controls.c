#include "controls.h"

void controls_update(controls_t* controls, const buttons_t* now) {
  const buttons_t* before = &controls->held;
  buttons_t pressed;
  pressed.left = now->left && !before->left;
  pressed.right = now->right && !before->right;
  pressed.up = now->up && !before->up;
  pressed.down = now->down && !before->down;
  pressed.jump = now->jump && !before->jump;
  pressed.run = now->run && !before->run;
  pressed.shoot = now->shoot && !before->shoot;
  pressed.melee = now->melee && !before->melee;
  pressed.drink = now->drink && !before->drink;
  pressed.pause = now->pause && !before->pause;
  pressed.confirm = now->confirm && !before->confirm;
  pressed.restart = now->restart && !before->restart;
  pressed.fullscreen = now->fullscreen && !before->fullscreen;
  pressed.mute = now->mute && !before->mute;
  controls->pressed = pressed;
  controls->held = *now;
}
