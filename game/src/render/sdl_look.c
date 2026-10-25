#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "look.h"
#include "manifest.h"

static bool read_file(const char* path, char** text) {
  FILE* file = fopen(path, "rb");
  if (!file) return false;
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  *text = malloc((size_t)size + 1);
  if (!*text) {
    fclose(file);
    return false;
  }
  size_t got = fread(*text, 1, (size_t)size, file);
  (*text)[got] = '\0';
  fclose(file);
  return true;
}

static texture_t load_image(graphics_context_ptr gfx, const char* path,
                            bool smooth) {
  texture_t texture = load_texture(gfx->renderer, path);
  set_texture_smooth(&texture, smooth);
  return texture;
}

static bool load_strip(look_t* look, graphics_context_ptr gfx, const char* dir,
                       const manifest_entry_t* entries, int count, int id) {
  const strip_spec_t* spec = &kStripSpecs[id];
  int frames = spec->frames > 0 ? spec->frames : 1;
  look->frames[id] = calloc((size_t)frames, sizeof(sprite_t));
  if (!look->frames[id]) return false;
  for (int f = 0; f < frames; f++) {
    char name[64], path[512];
    if (spec->frames > 0)
      snprintf(name, sizeof(name), "%s_%d", spec->prefix, f);
    else
      snprintf(name, sizeof(name), "%s", spec->prefix);
    const manifest_entry_t* entry = manifest_find(entries, count, name);
    if (!entry) {
      fprintf(stderr, "sprite %s is not in the manifest\n", name);
      return false;
    }
    snprintf(path, sizeof(path), "%s/%s", dir, entry->file);
    sprite_t* sprite = &look->frames[id][f];
    sprite->texture = load_image(gfx, path, true);
    if (!sprite->texture.texture) {
      fprintf(stderr, "cannot load %s\n", path);
      return false;
    }
    sprite->w = entry->w;
    sprite->h = entry->h;
    sprite->anchor_x = entry->anchor_x;
    sprite->anchor_y = entry->anchor_y;
  }
  return true;
}

static texture_t from_image(graphics_context_ptr gfx, image_t* image, bool smooth, bool additive) {
  texture_t texture = {NULL, 0, 0};
  if (image->pixels)
    texture = texture_from_pixels(gfx, image->pixels, image->w, image->h,
                                      smooth, additive);
  image_free(image);
  return texture;
}

bool look_load(look_t* look, graphics_context_ptr gfx, const char* sprite_dir) {
  memset(look, 0, sizeof(*look));
  look->font = build_pixel_font(gfx);
  char path[512];
  snprintf(path, sizeof(path), "%s/manifest.txt", sprite_dir);
  char* text = NULL;
  if (!read_file(path, &text)) {
    fprintf(stderr, "cannot read %s: run `make assets` first\n", path);
    return false;
  }
  manifest_entry_t entries[MANIFEST_MAX];
  int count = manifest_parse(text, entries, MANIFEST_MAX);
  free(text);
  if (count <= 0) {
    fprintf(stderr, "%s is empty or malformed\n", path);
    return false;
  }
  for (int id = 0; id < STRIP_COUNT; id++)
    if (!load_strip(look, gfx, sprite_dir, entries, count, id)) return false;

  image_t image;
  if (scenery_sky(&image, 128)) look->sky = from_image(gfx, &image, true, false);
  for (int layer = 0; layer < MOUNTAIN_LAYERS; layer++)
    if (scenery_layer(&image, layer))
      look->layers[layer] = from_image(gfx, &image, false, false);
  for (int i = 0; i < CLOUD_VARIANTS; i++)
    if (scenery_cloud(&image, (uint32_t)(11 + i * 7)))
      look->clouds[i] = from_image(gfx, &image, false, false);
  for (int kind = 0; kind < 3; kind++)
    if (scenery_tree(&image, (decor_kind_t)kind, 5u + (uint32_t)kind))
      look->trees[kind] = from_image(gfx, &image, false, false);
  if (scenery_castle(&image)) look->castle = from_image(gfx, &image, false, false);
  if (scenery_glow(&image)) look->glow = from_image(gfx, &image, true, true);
  snprintf(path, sizeof(path), "%s/cover.png", sprite_dir);
  look->cover = load_image(gfx, path, true);
  return look->sky.texture && look->castle.texture && look->glow.texture;
}

void look_free(look_t* look) {
  for (int id = 0; id < STRIP_COUNT; id++) {
    int frames = kStripSpecs[id].frames > 0 ? kStripSpecs[id].frames : 1;
    if (look->frames[id])
      for (int f = 0; f < frames; f++) free_texture(&look->frames[id][f].texture);
    free(look->frames[id]);
  }
  free_texture(&look->sky);
  for (int i = 0; i < MOUNTAIN_LAYERS; i++) free_texture(&look->layers[i]);
  for (int i = 0; i < CLOUD_VARIANTS; i++) free_texture(&look->clouds[i]);
  for (int i = 0; i < 3; i++) free_texture(&look->trees[i]);
  free_texture(&look->castle);
  free_texture(&look->glow);
  free_texture(&look->cover);
  free_pixel_font(&look->font);
  memset(look, 0, sizeof(*look));
}

const sprite_t* look_sprite(const look_t* look, pose_t pose) {
  return &look->frames[pose.strip][pose.frame];
}
