#include "pose.h"

#include <math.h>

const strip_spec_t kStripSpecs[STRIP_COUNT] = {
    [STRIP_FAB_IDLE] = {"fab_idle", 3},
    [STRIP_FAB_WALK] = {"fab_walkA", 10},
    [STRIP_FAB_RUN] = {"fab_run", 8},
    [STRIP_FAB_JUMP] = {"fab_jump", 5},
    [STRIP_FAB_ATTACK] = {"fab_attack", 4},
    [STRIP_FAB_HURT] = {"fab_hurt", 4},
    [STRIP_FAB_GUNWALK] = {"fab_gunwalk", 5},
    [STRIP_FAB_GUNAIM] = {"fab_gunaim", 0},
    [STRIP_FAB_GUNSHOOT] = {"fab_gunshoot", 0},
    [STRIP_FAB_POTION] = {"fab_potion", 2},
    [STRIP_FAB_DRINK] = {"fab_drink", 0},
    [STRIP_FAB_SPELL] = {"fab_spell", 0},
    [STRIP_FAB_CAST] = {"fab_cast", 4},
    [STRIP_ZOMBIE] = {"zombie", 4},
    [STRIP_ZOMBIE_ATTACK] = {"zombie_atk", 2},
    [STRIP_BUG_BEETLE] = {"bug_beetle", 0},
    [STRIP_BUG_FLY] = {"bug_fly", 0},
    [STRIP_BUG_BEE] = {"bug_bee", 0},
    [STRIP_BUG_DARK] = {"bug_dark", 0},
    [STRIP_BUG_SMALL] = {"bug_small", 0},
    [STRIP_FX_MUZZLE] = {"fx_muzzle", 0},
    [STRIP_FX_BULLET] = {"fx_bullet", 0},
    [STRIP_FX_RAY] = {"fx_ray", 0},
    [STRIP_FX_BUG_SPLAT] = {"fx_bug_splat", 0},
    [STRIP_FX_BUG_BOOM] = {"fx_bug_boom", 0},
    [STRIP_FX_BUG_BLUE] = {"fx_bug_blue", 0},
    [STRIP_FX_ZOMBIE_GREEN] = {"fx_zombie_green", 0},
    [STRIP_FX_ZOMBIE_BOOM] = {"fx_zombie_boom", 0},
    [STRIP_FX_SMOKE] = {"fx_smoke", 0},
    [STRIP_FX_REMAINS] = {"fx_remains", 0},
    [STRIP_PISTOL] = {"pistol_0", 0},
    [STRIP_BOTTLE] = {"bottle", 0},
    [STRIP_COIN] = {"coin", 0},
    [STRIP_MUSHROOM] = {"mushroom", 0},
    [STRIP_TILE_SOLID] = {"tile_solid", 0},
    [STRIP_TILE_Q] = {"tile_q", 0},
    [STRIP_TILE_BRICK] = {"tile_brick", 0},
    [STRIP_TILE_GOLD] = {"tile_gold", 0},
    [STRIP_TILE_STONE] = {"tile_stone", 0},
    [STRIP_GROUND_GRASS] = {"ground_grass", 0},
    [STRIP_GROUND_DIRT] = {"ground_dirt", 0},
    [STRIP_GROUND_ROCK] = {"ground_rock", 0},
    [STRIP_PIPE_CAP] = {"pipe_cap", 0},
    [STRIP_PIPE_BODY] = {"pipe_body", 0},
    [STRIP_BUBBLE] = {"bubble_a", 0},
};

static pose_t make(strip_id_t strip, int frame) {
  pose_t pose = {strip, frame};
  int count = kStripSpecs[strip].frames;
  if (count > 0) pose.frame = ((frame % count) + count) % count;
  else pose.frame = 0;
  return pose;
}

pose_t pose_player(const player_t* p) {
  if (!p->alive) return make(STRIP_FAB_HURT, 3);
  if (p->hurt_timer > 0.0f) return make(STRIP_FAB_HURT, 1);
  if (p->drink_timer > 0.15f) return make(STRIP_FAB_DRINK, 0);
  if (p->drink_timer > 0.0f) return make(STRIP_FAB_POTION, 0);
  if (p->melee_timer > 0.0f) {
    int frame = (int)((1.0f - p->melee_timer / 0.34f) * 4.0f);
    return make(STRIP_FAB_ATTACK, frame < 0 ? 0 : (frame > 3 ? 3 : frame));
  }
  if (p->cast_pose > 0.0f) return make(STRIP_FAB_SPELL, 0);
  if (p->shoot_pose > 0.12f) return make(STRIP_FAB_GUNSHOOT, 0);
  if (p->shoot_pose > 0.0f) return make(STRIP_FAB_GUNAIM, 0);

  bool moving = fabsf(p->vx) > 12.0f;
  if (!p->grounded) {
    if (p->has_gun) return make(STRIP_FAB_GUNWALK, 2);
    int frame = p->vy < -240.0f ? 1 : (p->vy < 120.0f ? 2 : (p->vy < 520.0f ? 3 : 4));
    return make(STRIP_FAB_JUMP, frame);
  }
  if (!moving) {
    if (p->has_gun) return make(STRIP_FAB_GUNAIM, 0);
    static const int breathing[4] = {0, 1, 2, 1};
    return make(STRIP_FAB_IDLE, breathing[(int)(p->anim_time * 2.5f) % 4]);
  }
  if (p->has_gun) return make(STRIP_FAB_GUNWALK, (int)p->walk_phase);
  if (fabsf(p->vx) > 230.0f) return make(STRIP_FAB_RUN, (int)(p->walk_phase * 0.8f));
  return make(STRIP_FAB_WALK, (int)p->walk_phase);
}

pose_t pose_zombie(const enemy_t* z, const player_t* p) {
  if (fabsf(z->x - p->x) < 46.0f && fabsf(z->y - p->y) < 40.0f)
    return make(STRIP_ZOMBIE_ATTACK, (int)(z->anim_time * 4.0f));
  static const int shuffle[4] = {0, 1, 3, 1};
  return make(STRIP_ZOMBIE, shuffle[(int)(z->anim_time * 4.5f) % 4]);
}

pose_t pose_bug(const enemy_t* b) {
  static const strip_id_t strips[BUG_VARIANTS] = {
      STRIP_BUG_BEETLE, STRIP_BUG_FLY, STRIP_BUG_BEE, STRIP_BUG_DARK,
      STRIP_BUG_SMALL};
  return make(strips[b->variant], 0);
}

pose_t pose_effect(fx_kind_t kind) {
  switch (kind) {
    case FX_ZOMBIE_BOOM: return make(STRIP_FX_ZOMBIE_BOOM, 0);
    case FX_ZOMBIE_GREEN: return make(STRIP_FX_ZOMBIE_GREEN, 0);
    case FX_BUG_BOOM: return make(STRIP_FX_BUG_BOOM, 0);
    case FX_BUG_BLUE: return make(STRIP_FX_BUG_BLUE, 0);
    case FX_SMOKE: return make(STRIP_FX_SMOKE, 0);
    case FX_REMAINS: return make(STRIP_FX_REMAINS, 0);
    case FX_MUZZLE: return make(STRIP_FX_MUZZLE, 0);
    case FX_BUG_SPLAT: return make(STRIP_FX_BUG_SPLAT, 0);
  }
  return make(STRIP_FX_SMOKE, 0);
}
