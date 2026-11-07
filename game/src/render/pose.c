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
    [STRIP_TILE_Q_BLUE] = {"tile_q_blue", 16},
    [STRIP_TILE_Q_GREY] = {"tile_q_grey", 16},
    [STRIP_TILE_Q_ORANGE] = {"tile_q_orange", 16},
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
    [STRIP_SKEWER] = {"skewer", 0},
    [STRIP_HELI_REST] = {"heli_rest", 0},
    [STRIP_FAB_HELI] = {"fab_heli", 3},
    [STRIP_HELI_BOOM] = {"heli_boom", 7},
    [STRIP_DRUMS_REST] = {"drums_rest", 0},
    [STRIP_DRUMS_PLAY] = {"drums_play", DRUM_FRAMES},
    [STRIP_FAB_BUB_IDLE] = {"fab_bub_idle", 5},
    [STRIP_FAB_BUB_WALK] = {"fab_bub_walk", 8},
    [STRIP_FAB_BUB_RUN] = {"fab_bub_run", 8},
    [STRIP_FAB_BUB_JUMP] = {"fab_bub_jump", 5},
    [STRIP_FAB_BUB_AIM] = {"fab_bub_aim", 5},
    [STRIP_FAB_BUB_SHOOT] = {"fab_bub_shoot", 8},
    [STRIP_BUBBLE_GUN] = {"bubble_gun_0", 0},
    [STRIP_SLOP_IDLE] = {"slop_idle", 4},
    [STRIP_SLOP_WINDUP] = {"slop_windup", 4},
    [STRIP_SLOP_THROW] = {"slop_throw", 4},
    [STRIP_SLOP_DEATH] = {"slop_death", 8},
    [STRIP_SLOP_BALL] = {"slop_ball", 6},
    [STRIP_SLOP_SPLAT] = {"slop_splat", 6},
    [STRIP_SLOP_PIECE] = {"slop_piece", SLOP_PIECES},
    [STRIP_FAB_JEDI_IDLE] = {"fab_jedi_idle", 5},
    [STRIP_FAB_JEDI_WALK] = {"fab_jedi_walk", 8},
    [STRIP_FAB_JEDI_RUN] = {"fab_jedi_run", 8},
    [STRIP_FAB_JEDI_JUMP] = {"fab_jedi_jump", 5},
    [STRIP_FAB_JEDI_SWING] = {"fab_jedi_swing", 6},
    [STRIP_FAB_JEDI_HURT] = {"fab_jedi_hurt", 2},
    [STRIP_SABER] = {"saber", 4},
    [STRIP_FORCE_BURST] = {"force_burst", 6},
    [STRIP_FORCE_HIT] = {"force_hit", 6},
    [STRIP_OLD_KNIGHT] = {"old_knight", 2},
    [STRIP_GROUND_SNOW] = {"ground_snow", 4},
    [STRIP_FILL_SNOW] = {"fill_snow", 4},
    [STRIP_GROUND_SAND] = {"ground_sand", 4},
    [STRIP_FILL_SAND] = {"fill_sand", 4},
    [STRIP_GROUND_CAVE] = {"ground_cave", 4},
    [STRIP_FILL_CAVE] = {"fill_cave", 4},
    [STRIP_DECOR_SNOW_PINE] = {"decor_snow_pine", 3},
    [STRIP_DECOR_SNOWMAN] = {"decor_snowman", 0},
    [STRIP_DECOR_ICE] = {"decor_ice", 0},
    [STRIP_DECOR_FROST_BUSH] = {"decor_frost_bush", 0},
    [STRIP_DECOR_CACTUS] = {"decor_cactus", 3},
    [STRIP_DECOR_DEAD_TREE] = {"decor_dead_tree", 0},
    [STRIP_DECOR_SKULL] = {"decor_skull", 0},
    [STRIP_DECOR_BOULDER] = {"decor_boulder", 0},
    [STRIP_DECOR_CRYSTAL] = {"decor_crystal", 3},
    [STRIP_DECOR_MUSHROOM] = {"decor_mushroom", 0},
    [STRIP_DECOR_STALAGMITE] = {"decor_stalagmite", 2},
    [STRIP_DECOR_STALACTITE] = {"decor_stalactite", 5},
    [STRIP_DECOR_CAR] = {"city_car", 4},
    [STRIP_DECOR_OVID] = {"city_ovid", 0},
    [STRIP_DECOR_FOUNTAIN] = {"city_fountain", 2},
    [STRIP_ALLY_ANDREA_IDLE] = {"ally_andrea_idle", 4},
    [STRIP_ALLY_ANDREA_WALK] = {"ally_andrea_walk", 6},
    [STRIP_ALLY_ANDREA_JUMP] = {"ally_andrea_jump", 3},
    [STRIP_ALLY_ANDREA_ATTACK] = {"ally_andrea_attack", 6},
    [STRIP_ALLY_ANDREA_FX] = {"ally_andrea_fx", 4},
    [STRIP_ALLY_ALESSANDRO_IDLE] = {"ally_alessandro_idle", 4},
    [STRIP_ALLY_ALESSANDRO_WALK] = {"ally_alessandro_walk", 6},
    [STRIP_ALLY_ALESSANDRO_JUMP] = {"ally_alessandro_jump", 3},
    [STRIP_ALLY_ALESSANDRO_ATTACK] = {"ally_alessandro_attack", 6},
    [STRIP_ALLY_ALESSANDRO_FX] = {"ally_alessandro_fx", 4},
    [STRIP_ALLY_GIULIA_IDLE] = {"ally_giulia_idle", 4},
    [STRIP_ALLY_GIULIA_WALK] = {"ally_giulia_walk", 6},
    [STRIP_ALLY_GIULIA_JUMP] = {"ally_giulia_jump", 3},
    [STRIP_ALLY_GIULIA_ATTACK] = {"ally_giulia_attack", 6},
    [STRIP_ALLY_GIULIA_FX] = {"ally_giulia_fx", 4},
    [STRIP_ITEM_SUSHI] = {"item_sushi", 4},
    [STRIP_ITEM_DRAGON] = {"item_dragon", 4},
    [STRIP_ITEM_GLOVES] = {"item_gloves", 4},
    [STRIP_ALLY_POOF] = {"ally_poof", 6},
    [STRIP_SOAP_BUBBLE] = {"bubble_big", 0},
    [STRIP_CHUTE_FALL] = {"chute_fall", 3},
    [STRIP_CHUTE_SHOOT] = {"chute_shoot", 3},
    [STRIP_CHUTE_OPEN] = {"chute_open", 3},
    [STRIP_CHUTE_LAND] = {"chute_land", 4},
    [STRIP_AQUEDUCT] = {"city_aqueduct", 0},
    [STRIP_TAP] = {"city_tap", 4},
    [STRIP_WATER] = {"city_water", 4},
    [STRIP_SPLASH] = {"city_splash", 4},
    [STRIP_VASE] = {"city_vase", 3},
    [STRIP_THAI_WINDOW] = {"thai_window", 6},
    [STRIP_THAI_WINDOW_B] = {"thai_window_b", 6},
    [STRIP_THAI_SIGN] = {"thai_sign", 4},
    [STRIP_LANTERN] = {"thai_lantern", 3},
    [STRIP_FLOWER_BOX] = {"thai_flowers", 2},
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
  if (p->chute) { /* the parachute: opening, then swaying down, shooting when he does */
    if (p->shoot_pose > 0.0f) return make(STRIP_CHUTE_SHOOT, 1 + (int)(p->anim_time * 20.0f) % 2);
    if (p->chute_age < PARACHUTE_OPEN_SECONDS) return make(STRIP_CHUTE_OPEN, (int)(p->chute_age / PARACHUTE_OPEN_SECONDS * 3.0f));
    static const int sway[4] = {0, 1, 2, 1};
    return make(STRIP_CHUTE_FALL, sway[(int)(p->anim_time * 2.0f) % 4]);
  }
  if (p->landing > 0.0f && p->grounded) return make(STRIP_CHUTE_LAND, p->landing > PARACHUTE_LANDING_SECONDS * 0.5f ? 0 : 1);
  if (p->flying) return make(STRIP_FAB_HELI, (int)(p->anim_time * 18.0f)); /* the rotor spins */
  if (p->hurt_timer > 0.0f) return p->jedi_time > 0.0f ? make(STRIP_FAB_JEDI_HURT, 0) : make(STRIP_FAB_HURT, 1);
  if (p->jedi_time > 0.0f) { /* the knight's robes, for as long as the Force is with him */
    if (p->shoot_pose > 0.0f) return make(STRIP_FAB_JEDI_SWING, (int)((1.0f - p->shoot_pose / 0.3f) * 6.0f));
    if (!p->grounded) return make(STRIP_FAB_JEDI_JUMP, p->vy < -240.0f ? 1 : (p->vy < 120.0f ? 2 : (p->vy < 520.0f ? 3 : 4)));
    if (fabsf(p->vx) <= 12.0f) return make(STRIP_FAB_JEDI_IDLE, (int)(p->anim_time * 2.5f));
    return make(fabsf(p->vx) > 230.0f ? STRIP_FAB_JEDI_RUN : STRIP_FAB_JEDI_WALK, (int)p->walk_phase);
  }
  if (p->drink_timer > 0.15f) return make(STRIP_FAB_DRINK, 0);
  if (p->drink_timer > 0.0f) return make(STRIP_FAB_POTION, 0);
  if (p->melee_timer > 0.0f) {
    int frame = (int)((1.0f - p->melee_timer / 0.34f) * 4.0f);
    return make(STRIP_FAB_ATTACK, frame < 0 ? 0 : (frame > 3 ? 3 : frame));
  }
  if (p->cast_pose > 0.0f) return make(STRIP_FAB_SPELL, 0);
  if (p->bubbles > 0) { /* the shotgun is in hand: it takes the pistol's place */
    if (p->shoot_pose > 0.0f) return make(STRIP_FAB_BUB_SHOOT, 1 + (int)(p->anim_time * 20.0f) % 2);
    if (!p->grounded) return make(STRIP_FAB_BUB_JUMP, p->vy < -240.0f ? 1 : (p->vy < 120.0f ? 2 : (p->vy < 520.0f ? 3 : 4)));
    if (fabsf(p->vx) <= 12.0f) return make(STRIP_FAB_BUB_IDLE, (int)(p->anim_time * 2.5f));
    return make(fabsf(p->vx) > 230.0f ? STRIP_FAB_BUB_RUN : STRIP_FAB_BUB_WALK, (int)p->walk_phase);
  }
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

pose_t pose_drums(const drum_kit_t* kit, float time) {
  if (kit->playing <= 0.0f) return make(STRIP_DRUMS_REST, 0);
  return make(STRIP_DRUMS_PLAY, (int)(time / DRUM_FRAME_SECONDS));
}

pose_t pose_helicopter_boom(float age) {
  int frame = (int)(age / (HELICOPTER_BOOM_SECONDS / 7.0f));
  return make(STRIP_HELI_BOOM, frame > 6 ? 6 : frame);
}

int question_block_colour(int col, int row) {
  unsigned h = (unsigned)col * 2654435761u + (unsigned)row * 40503u;
  return (int)((h >> 13) % QBLOCK_COLOURS);
}

pose_t pose_question_block(float time, int col, int row) {
  /* It turns round in a second, then rests a moment facing out. */
  strip_id_t strip = (strip_id_t)(STRIP_TILE_Q + question_block_colour(col, row));
  float in_cycle = fmodf(time, QBLOCK_SPIN_SECONDS + QBLOCK_REST_SECONDS);
  if (in_cycle >= QBLOCK_SPIN_SECONDS) return make(strip, 0);
  return make(strip, (int)(in_cycle / QBLOCK_SPIN_SECONDS * 16.0f));
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
    case FX_HELICOPTER_BOOM: return make(STRIP_HELI_BOOM, 0);
    case FX_ALLY_POOF: return make(STRIP_ALLY_POOF, 0);
    case FX_ALLY_WAVE: return make(STRIP_ALLY_ANDREA_FX, 0);
    case FX_ALLY_SLASH: return make(STRIP_ALLY_ALESSANDRO_FX, 0);
    case FX_ALLY_PUNCH: return make(STRIP_ALLY_GIULIA_FX, 0);
    case FX_FORCE_HIT: return make(STRIP_FORCE_HIT, 0);
    case FX_SLOP_DEATH: return make(STRIP_SLOP_DEATH, 0);
    case FX_SLOP_SPLAT: return make(STRIP_SLOP_SPLAT, 0);
    case FX_CHUTE_REMAINS: return make(STRIP_CHUTE_LAND, 2);
  }
  return make(STRIP_FX_SMOKE, 0);
}

pose_t pose_slop(const enemy_t* s) {
  float into;
  switch (slop_act(s, &into)) {
    case SLOP_WINDING_UP: return make(STRIP_SLOP_WINDUP, (int)(into / (SLOP_WINDUP_SECONDS / 4.0f)));
    case SLOP_THROWING: {
      int frame = (int)(into / (SLOP_RELEASE / 2.0f)); /* the release is the third frame */
      return make(STRIP_SLOP_THROW, frame > 3 ? 3 : frame);
    }
    case SLOP_STANDING: break;
  }
  return make(STRIP_SLOP_IDLE, (int)(s->anim_time * 4.0f));
}

pose_t pose_slop_effect(fx_kind_t kind, float age, float duration) {
  strip_id_t strip = kind == FX_SLOP_DEATH ? STRIP_SLOP_DEATH : STRIP_SLOP_SPLAT;
  return make(strip, (int)(age / duration * (float)kStripSpecs[strip].frames));
}

pose_t pose_force_burst(float age) { return make(STRIP_FORCE_BURST, (int)(age * 24.0f)); }

pose_t pose_force_hit(float age, float duration) {
  return make(STRIP_FORCE_HIT, (int)(age / duration * (float)kStripSpecs[STRIP_FORCE_HIT].frames));
}

pose_t pose_old_knight(float left) {
  float age = GREETING_SECONDS - left;
  bool speaking = age > GREETING_FADE_IN && left > GREETING_FADE_OUT;
  return make(STRIP_OLD_KNIGHT, speaking ? (int)(age * 7.0f) % 2 : 0);
}

pose_t pose_masseuse(int window, float time) {
  return make(window ? STRIP_THAI_WINDOW_B : STRIP_THAI_WINDOW, (int)(time * 1.6f));
}

pose_t pose_decor(decor_kind_t kind, int variant) {
  return make((strip_id_t)(STRIP_DECOR_SNOW_PINE + (kind - DECOR_SNOW_PINE)), variant);
}

pose_t pose_ground(biome_t biome, bool fill, int col) {
  strip_id_t strip = biome == BIOME_SNOW ? STRIP_GROUND_SNOW
                     : biome == BIOME_DESERT ? STRIP_GROUND_SAND : STRIP_GROUND_CAVE;
  return make((strip_id_t)(strip + (fill ? 1 : 0)), col);
}

pose_t pose_ally(const ally_t* a) {
  strip_id_t base = (strip_id_t)(STRIP_ALLY_ANDREA_IDLE + a->kind * 5);
  if (a->attack > 0.0f)
    return make((strip_id_t)(base + 3), (int)(a->attack / ally_attack_seconds(a->kind) * 6.0f));
  if (!a->grounded) return make((strip_id_t)(base + 2), a->vy < -200.0f ? 0 : (a->vy < 200.0f ? 1 : 2));
  if (fabsf(a->vx) > 12.0f) return make((strip_id_t)(base + 1), (int)(a->anim * 11.0f));
  return make(base, (int)(a->anim * 3.0f));
}

pose_t pose_ally_effect(fx_kind_t kind, float age, float duration) {
  strip_id_t strip = kind == FX_ALLY_POOF ? STRIP_ALLY_POOF
                     : kind == FX_ALLY_WAVE ? STRIP_ALLY_ANDREA_FX
                     : kind == FX_ALLY_SLASH ? STRIP_ALLY_ALESSANDRO_FX : STRIP_ALLY_GIULIA_FX;
  return make(strip, (int)(age / duration * (float)kStripSpecs[strip].frames));
}

pose_t pose_ally_item(ally_kind_t kind, float time) {
  return make((strip_id_t)(STRIP_ITEM_SUSHI + kind), (int)(time * 6.0f));
}
