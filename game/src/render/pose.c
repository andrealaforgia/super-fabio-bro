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
    [STRIP_TILE_Q] = {"tile_q", 16},
    [STRIP_TILE_BRICK] = {"tile_brick", 0},
    [STRIP_TILE_GOLD] = {"tile_gold", 0},
    [STRIP_TILE_STONE] = {"tile_stone", 0},
    [STRIP_GROUND_GRASS] = {"ground_grass", 0},
    [STRIP_GROUND_DIRT] = {"ground_dirt", 0},
    [STRIP_GROUND_ROCK] = {"ground_rock", 0},
    [STRIP_PIPE_CAP] = {"pipe_cap", 0},
    [STRIP_PIPE_BODY] = {"pipe_body", 0},
    [STRIP_BUBBLE] = {"bubble_a", 0},
    [STRIP_READER_READ] = {"reader_read", 3},
    [STRIP_READER_LAUGH] = {"reader_laugh", 8},
    [STRIP_READER_POP] = {"reader_pop", 7},
    [STRIP_READER_FLASH] = {"reader_flash", 0},
    [STRIP_READER_BOOM] = {"reader_boom", 0},
    [STRIP_READER_REMAINS] = {"reader_remains", 0},
    [STRIP_MONK_CROSS] = {"monk_cross", 4},
    [STRIP_MONK_BOOM] = {"monk_boom", 15},
    [STRIP_SKELETON_WALK] = {"skeleton_walk", 8},
    [STRIP_SKELETON_VANISH] = {"skeleton_vanish", 11},
    [STRIP_BUBBLE_MONK] = {"bubble_monk", 0},
    [STRIP_BUBBLE_SKELETON] = {"bubble_skeleton", 0},
    [STRIP_CHEF_HAPPY] = {"chef_happy", 2},
    [STRIP_CHEF_ATTACK] = {"chef_attack", 4},
    [STRIP_CHEF_BOOM] = {"chef_boom", 2},
    [STRIP_BUBBLE_CHEF] = {"bubble_chef", 0},
    [STRIP_GORILLA_IDLE] = {"gorilla_idle", 0},
    [STRIP_GORILLA_ROAR] = {"gorilla_roar", 4},
    [STRIP_GORILLA_BEAT] = {"gorilla_beat", 2},
    [STRIP_GORILLA_THROW] = {"gorilla_throw", 4},
    [STRIP_BANANA] = {"banana", 0},
    [STRIP_DEMON_STAND] = {"demon_stand", 4},
    [STRIP_DEMON_CAST] = {"demon_cast", 4},
    [STRIP_DEMON_DEATH] = {"demon_death", 8},
    [STRIP_FIREBALL] = {"demon_fireball", 0},
    [STRIP_VEGETABLE] = {"vegetable", VEGETABLE_KINDS},
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
    if (p->ammo > 0) return make(STRIP_FAB_GUNWALK, 2);
    int frame = p->vy < -240.0f ? 1 : (p->vy < 120.0f ? 2 : (p->vy < 520.0f ? 3 : 4));
    return make(STRIP_FAB_JUMP, frame);
  }
  if (!moving) {
    if (p->ammo > 0) return make(STRIP_FAB_GUNAIM, 0);
    static const int breathing[4] = {0, 1, 2, 1};
    return make(STRIP_FAB_IDLE, breathing[(int)(p->anim_time * 2.5f) % 4]);
  }
  if (p->ammo > 0) return make(STRIP_FAB_GUNWALK, (int)p->walk_phase);
  /* The generated walk frames barely differ from one another; the run frames
   * swing arms and legs, so they carry walking too. */
  return make(STRIP_FAB_RUN, (int)p->walk_phase);
}

pose_t pose_monk(const enemy_t* m, const player_t* p) {
  float away = fabsf(p->x - m->x);
  if (fabsf(p->y - m->y) > 140.0f || away >= MONK_CROSS_RANGE) return make(STRIP_MONK_CROSS, 0);
  if (away >= MONK_CROSS_RANGE * 0.7f) return make(STRIP_MONK_CROSS, 1);
  if (away >= MONK_CROSS_RANGE * 0.45f) return make(STRIP_MONK_CROSS, 2);
  return make(STRIP_MONK_CROSS, 3);
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

pose_t pose_book_reader(const enemy_t* m) {
  if (book_reader_mood(m) == BOOK_READER_READING) {
    static const int turning[4] = {0, 1, 2, 1};
    return make(STRIP_READER_READ, turning[(int)(m->anim_time * 2.2f) % 4]);
  }
  static const int roaring[10] = {2, 3, 4, 5, 6, 7, 6, 5, 4, 3};
  return make(STRIP_READER_LAUGH, roaring[(int)(m->anim_time * 10.0f) % 10]);
}

pose_t pose_book_reader_boom(float age) {
  if (age < 0.56f) return make(STRIP_READER_POP, (int)(age / 0.08f));
  if (age < 0.66f) return make(STRIP_READER_FLASH, 0);
  if (age < 1.05f) return make(STRIP_READER_BOOM, 0);
  return make(STRIP_READER_REMAINS, 0);
}

pose_t pose_monk_boom(float age) {
  /* Fast through the blast, slower as it thins to smoke, then the debris. */
  if (age < 0.54f) return make(STRIP_MONK_BOOM, (int)(age / 0.09f));
  if (age < 1.14f) return make(STRIP_MONK_BOOM, 6 + (int)((age - 0.54f) / 0.12f));
  int frame = 11 + (int)((age - 1.14f) / 0.15f);
  return make(STRIP_MONK_BOOM, frame > 14 ? 14 : frame);
}

pose_t pose_skeleton(const enemy_t* s) {
  if (fabsf(s->vx) < 1.0f) return make(STRIP_SKELETON_WALK, 0);
  return make(STRIP_SKELETON_WALK, (int)(s->anim_time * 7.0f));
}

pose_t pose_skeleton_vanish(float age) {
  int frame = (int)(age / 0.045f);
  return make(STRIP_SKELETON_VANISH, frame > 10 ? 10 : frame);
}

pose_t pose_chef(const enemy_t* c) {
  if (chef_attacking(c)) return make(STRIP_CHEF_ATTACK, chef_swing_frame(c));
  /* Two happy frames, arms up and arms down. */
  return make(STRIP_CHEF_HAPPY, (int)(c->anim_time * 3.0f));
}

pose_t pose_chef_death(float age) {
  return make(STRIP_CHEF_BOOM, age < 0.25f ? 0 : 1);
}

pose_t pose_gorilla(const enemy_t* g) {
  float into;
  switch (gorilla_act(g, &into)) {
    case GORILLA_ROARING:
      return make(STRIP_GORILLA_ROAR, (int)(into / (GORILLA_ROAR_SECONDS / 4.0f)));
    case GORILLA_BEATING: return make(STRIP_GORILLA_BEAT, (int)(into / 0.09f) % 2);
    case GORILLA_THROWING: {
      int frame = (int)(into / (GORILLA_RELEASE / 3.0f));
      return make(STRIP_GORILLA_THROW, frame > 3 ? 3 : frame);
    }
    case GORILLA_STANDING: break;
  }
  return make(STRIP_GORILLA_IDLE, 0);
}

pose_t pose_demon(const enemy_t* d) {
  float into;
  if (demon_casting(d, &into)) {
    int frame = (int)(into / (DEMON_CAST_SECONDS / 4.0f));
    return make(STRIP_DEMON_CAST, frame > 3 ? 3 : frame);
  }
  return make(STRIP_DEMON_STAND, (int)(into * 4.0f));
}

pose_t pose_demon_death(float age) {
  int frame = (int)(age / (DEMON_DEATH_SECONDS / 8.0f));
  return make(STRIP_DEMON_DEATH, frame > 7 ? 7 : frame);
}

pose_t pose_question_block(float time) {
  /* It turns round in a second, then rests a moment facing out. */
  float in_cycle = fmodf(time, QBLOCK_SPIN_SECONDS + QBLOCK_REST_SECONDS);
  if (in_cycle >= QBLOCK_SPIN_SECONDS) return make(STRIP_TILE_Q, 0);
  return make(STRIP_TILE_Q, (int)(in_cycle / QBLOCK_SPIN_SECONDS * 16.0f));
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
    case FX_BOOK_READER_BOOM: return make(STRIP_READER_BOOM, 0);
    case FX_MONK_BOOM: return make(STRIP_MONK_BOOM, 0);
    case FX_SKELETON_VANISH: return make(STRIP_SKELETON_VANISH, 0);
    case FX_CHEF_EXPLODE: return make(STRIP_CHEF_BOOM, 0);
    case FX_DEMON_DEATH: return make(STRIP_DEMON_DEATH, 0);
  }
  return make(STRIP_FX_SMOKE, 0);
}
