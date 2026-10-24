/**
 * @file render.h
 * @brief Draws the world, the heads-up display and the title, pause, game
 * over and victory screens.
 */
#ifndef GAME_RENDER_RENDER_H_
#define GAME_RENDER_RENDER_H_

#include "look.h"
#include "world.h"

void render_world(gfx_t* gfx, const look_t* look, const world_t* world);
void render_title(gfx_t* gfx, const look_t* look, float time);
void render_pause(gfx_t* gfx);
/* A big countdown, for setting up a screen recording. */
void render_countdown(gfx_t* gfx, float seconds_left);
/* A line of text on a dark strip at the bottom of the screen. */
void render_caption(gfx_t* gfx, const char* text);

#endif  // GAME_RENDER_RENDER_H_
