/**
 * @file render.h
 * @brief Draws the world, the heads-up display and the title, pause, game
 * over and victory screens.
 */
#ifndef GAME_RENDER_RENDER_H_
#define GAME_RENDER_RENDER_H_

#include "look.h"
#include "world.h"

/* Drawing is in world units: the view is always this tall, and as wide as
 * the window's shape allows. */
#define VIEW_HEIGHT 544.0f

void render_world(graphics_context_ptr gfx, const look_t* look, const world_t* world);
void render_title(graphics_context_ptr gfx, const look_t* look, float time);
void render_pause(graphics_context_ptr gfx, const look_t* look);
/* A big countdown, for setting up a screen recording. */
void render_countdown(graphics_context_ptr gfx, const look_t* look,
                      float seconds_left);

#endif  // GAME_RENDER_RENDER_H_
