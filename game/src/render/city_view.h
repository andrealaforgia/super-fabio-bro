/**
 * @file city_view.h
 * @brief The cities, from London to Sulmona, drawn in code and from the city sprites: the buildings with their
 * balconies and roofs, the shops and signs, the London Eye, the aqueduct, its water, the tap and the vases.
 */
#ifndef GAME_RENDER_CITY_VIEW_H_
#define GAME_RENDER_CITY_VIEW_H_

#include "look.h"
#include "world.h"

/* The paving of a city street: `surface` for the top tile, otherwise the foundations. */
void city_draw_ground_tile(graphics_context_ptr gfx, float cam_x, float x, float y, bool surface, int col);
/* Behind everything: the buildings, and the aqueduct. */
void city_draw_back(graphics_context_ptr gfx, const look_t* look, const world_t* world, float cam_x, float view_w);
/* A code-drawn decoration (the welcome signs, the landmarks, the signpost). Returns false for any other kind. */
bool city_draw_decor(graphics_context_ptr gfx, const look_t* look, const world_t* world, const decor_t* decor, float cam_x);
/* In front of the street, behind the actors: the vases, the tap, the water, the hints. */
void city_draw_front(graphics_context_ptr gfx, const look_t* look, const world_t* world, float cam_x, float view_w);

#endif  // GAME_RENDER_CITY_VIEW_H_
