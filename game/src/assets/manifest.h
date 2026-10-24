/**
 * @file manifest.h
 * @brief The list of cooked sprites (assets_cooked/sprites/manifest.txt):
 * one line per sprite, "name file width height anchor_x anchor_y".
 */
#ifndef GAME_ASSETS_MANIFEST_H_
#define GAME_ASSETS_MANIFEST_H_

#define MANIFEST_MAX 256

typedef struct {
  char name[40];
  char file[48];
  int w, h;
  int anchor_x, anchor_y;
} manifest_entry_t;

/* Parses the text; returns the number of entries, or -1 on a malformed line. */
int manifest_parse(const char* text, manifest_entry_t* entries, int capacity);
const manifest_entry_t* manifest_find(const manifest_entry_t* entries,
                                      int count, const char* name);

#endif  // GAME_ASSETS_MANIFEST_H_
