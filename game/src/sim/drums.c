#include <math.h>

#include "world.h"

const char* const kDrumWords[DRUM_WORDS] = {"BOOM", "BADABOOM", "TASH", "BODOBOOOM", "BOING", "DISH"};

frect_t drum_kit_rect(const drum_kit_t* kit) {
  frect_t r = {kit->x - DRUM_W * 0.5f, kit->y - DRUM_H, DRUM_W, DRUM_H};
  return r;
}

float drum_word_radius(float age) {
  float t = clampf(age / DRUM_WORD_SECONDS, 0.0f, 1.0f);
  return DRUM_WORD_RADIUS * (1.0f - (1.0f - t) * (1.0f - t)); /* quick at first, then easing out */
}

bool world_drumming(const world_t* world) {
  for (int i = 0; i < world->drum_count; i++)
    if (world->drums[i].playing > 0.0f) return true;
  return false;
}

/* He has to jump into it: on the ground, walking past, he does nothing. */
static void sit_down(world_t* world, drum_kit_t* kit) {
  player_t* p = &world->player;
  kit->armed = false;
  kit->playing = DRUM_SECONDS; /* a fresh ten seconds every time, never added to the last */
  kit->next_word = 0.0f;
  p->x = kit->x;
  p->y = kit->y;
  p->vx = p->vy = 0.0f;
  p->grounded = true;
  p->platform = -1;
  world_emit(world, EVENT_DRUMS_START, p->x);
}

/* The gig is over: he hops out of the kit. */
static void stand_up(world_t* world, drum_kit_t* kit) {
  player_t* p = &world->player;
  kit->playing = 0.0f;
  p->vy = -320.0f;
  p->grounded = false;
  p->invulnerable = 1.0f;
}

static void shout(world_t* world, drum_kit_t* kit) {
  int word = rng_range(&world->rng, 0, DRUM_WORDS - 1);
  if (word == kit->last_word) word = (word + 1) % DRUM_WORDS;
  kit->last_word = word;
  drum_word_t* w = pool_acquire(&world->drum_words, NULL);
  if (w) *w = (drum_word_t){kit->x, kit->y - DRUM_H * 0.5f, 0.0f, word};
  world_emit(world, (event_kind_t)(EVENT_DRUM_BOOM + word), kit->x);
}

static bool circle_touches(float cx, float cy, float r, frect_t box) {
  float nx = clampf(cx, box.x, box.x + box.w), ny = clampf(cy, box.y, box.y + box.h);
  return (cx - nx) * (cx - nx) + (cy - ny) * (cy - ny) <= r * r;
}

/* Whatever stands in a word's ring dies. */
static void kill_inside(world_t* world, const drum_word_t* w) {
  float r = drum_word_radius(w->age);
  size_t cursor = 0;
  for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));) {
    frect_t box = {e->x - e->w * 0.5f, e->y - e->h, e->w, e->h};
    if (circle_touches(w->x, w->y, r, box)) world_kill_enemy(world, e, false);
  }
}

static void play(world_t* world, drum_kit_t* kit, float dt) {
  player_t* p = &world->player;
  kit->playing -= dt;
  p->x = kit->x;
  p->y = kit->y;
  kit->next_word -= dt;
  while (kit->next_word <= 0.0f && kit->playing > 0.0f) {
    shout(world, kit);
    kit->next_word += DRUM_WORD_EVERY;
  }
  if (kit->playing <= 0.0f) stand_up(world, kit);
}

void drums_update(world_t* world, float dt) {
  player_t* p = &world->player;
  for (int i = 0; i < world->drum_count; i++) {
    drum_kit_t* kit = &world->drums[i];
    bool inside = frect_overlaps(player_rect(p), drum_kit_rect(kit));
    if (!inside) kit->armed = true; /* hopping out of it does not drop him straight back in */
    if (kit->armed && !world_drumming(world) && p->alive && !p->flying && !p->grounded && inside)
      sit_down(world, kit);
    if (kit->playing > 0.0f) play(world, kit, dt);
  }

  size_t cursor = 0;
  for (drum_word_t* w; (w = pool_next_active(&world->drum_words, &cursor));) {
    w->age += dt;
    if (w->age > DRUM_WORD_SECONDS) {
      pool_release_object(&world->drum_words, w);
      continue;
    }
    kill_inside(world, w);
  }
}
