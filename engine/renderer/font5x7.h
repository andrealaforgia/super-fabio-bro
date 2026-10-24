/**
 * @file font5x7.h
 * @brief A built-in 5x7 pixel font for debug text: digits, capitals and
 * a little punctuation. Lowercase maps to capitals.
 */
#ifndef ENGINE_RENDERER_FONT5X7_H_
#define ENGINE_RENDERER_FONT5X7_H_

#include <stdbool.h>

#define FONT5X7_WIDTH 5
#define FONT5X7_HEIGHT 7

/* Row `row` (0 is the top) of the glyph, column `column` (0 is the left). */
bool font5x7_pixel(char c, int row, int column);
bool font5x7_has_glyph(char c);

#endif  // ENGINE_RENDERER_FONT5X7_H_
