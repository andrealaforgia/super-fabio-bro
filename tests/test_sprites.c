#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "manifest.h"
#include "pose.h"
#include "test_groups.h"
#include "test_support.h"

static void the_manifest_parser_reads_lines_and_rejects_junk(void) {
  manifest_entry_t entries[4];
  CHECK(manifest_parse("a a.png 10 20 5 19\nb b.png 3 4 1 3\n", entries, 4) == 2);
  CHECK(strcmp(entries[0].name, "a") == 0 && entries[0].w == 10);
  CHECK(entries[1].anchor_y == 3);
  CHECK(manifest_find(entries, 2, "b") == &entries[1]);
  CHECK(manifest_find(entries, 2, "zzz") == NULL);
  CHECK(manifest_parse("a a.png 10\n", entries, 4) == -1);
  CHECK(manifest_parse("a a.png 0 20 5 19\n", entries, 4) == -1);
  CHECK(manifest_parse("a a.png 1 1 0 0\n" "b b.png 1 1 0 0\n", entries, 1) == -1);
  CHECK(manifest_parse("", entries, 4) == 0);
}

static int load_cooked(manifest_entry_t* entries) {
  FILE* file = fopen("assets_cooked/sprites/manifest.txt", "rb");
  if (!file) {
    fprintf(stderr, "no cooked sprites: run `make assets`\n");
    exit(1);
  }
  static char text[65536];
  size_t n = fread(text, 1, sizeof(text) - 1, file);
  text[n] = '\0';
  fclose(file);
  int count = manifest_parse(text, entries, MANIFEST_MAX);
  CHECK(count > 0);
  return count;
}

static void every_sprite_the_game_draws_was_cooked(void) {
  manifest_entry_t entries[MANIFEST_MAX];
  int count = load_cooked(entries);
  for (int i = 0; i < STRIP_COUNT; i++) {
    const strip_spec_t* spec = &kStripSpecs[i];
    CHECK(spec->prefix != NULL);
    if (spec->frames == 0) {
      if (!manifest_find(entries, count, spec->prefix))
        fprintf(stderr, "missing %s\n", spec->prefix);
      CHECK(manifest_find(entries, count, spec->prefix) != NULL);
    }
    for (int f = 0; f < spec->frames; f++) {
      char name[64];
      snprintf(name, sizeof(name), "%s_%d", spec->prefix, f);
      if (!manifest_find(entries, count, name)) fprintf(stderr, "missing %s\n", name);
      CHECK(manifest_find(entries, count, name) != NULL);
    }
  }
}

static void cooked_sprites_have_sensible_sizes_and_anchors(void) {
  manifest_entry_t entries[MANIFEST_MAX];
  int count = load_cooked(entries);
  for (int i = 0; i < count; i++) {
    CHECK(entries[i].w >= 8 && entries[i].w <= 400);
    CHECK(entries[i].h >= 8 && entries[i].h <= 400);
    CHECK(entries[i].anchor_x >= 0 && entries[i].anchor_x < entries[i].w);
    CHECK(entries[i].anchor_y == entries[i].h - 1);
  }
}

static player_t standing(void) {
  player_t p = {0};
  p.alive = true;
  p.grounded = true;
  p.facing = 1;
  return p;
}

static void a_standing_fabio_breathes_through_the_idle_frames(void) {
  player_t p = standing();
  bool seen[3] = {false, false, false};
  for (float t = 0.0f; t < 2.0f; t += 0.05f) {
    p.anim_time = t;
    pose_t pose = pose_player(&p);
    CHECK(pose.strip == STRIP_FAB_IDLE);
    seen[pose.frame] = true;
  }
  CHECK(seen[0] && seen[1] && seen[2]);
}

static void walking_and_running_cycle_the_swinging_run_frames(void) {
  player_t p = standing();
  p.vx = 170.0f;
  p.walk_phase = 3.2f;
  CHECK(pose_player(&p).strip == STRIP_FAB_RUN);
  CHECK(pose_player(&p).frame == 3);
  p.walk_phase = 11.0f;
  CHECK(pose_player(&p).frame == 3);
  p.vx = 285.0f;
  p.walk_phase = 100.0f;
  CHECK(pose_player(&p).strip == STRIP_FAB_RUN && pose_player(&p).frame < 8);
}

static void walking_visits_every_frame_so_the_legs_really_move(void) {
  player_t p = standing();
  p.vx = 175.0f;
  bool seen[8] = {false};
  for (float phase = 0.0f; phase < 8.0f; phase += 0.1f) {
    p.walk_phase = phase;
    seen[pose_player(&p).frame] = true;
  }
  for (int i = 0; i < 8; i++) CHECK(seen[i]);
}

static void in_the_air_the_jump_frame_follows_the_climb_and_the_fall(void) {
  player_t p = standing();
  p.grounded = false;
  p.vy = -600.0f;
  CHECK(pose_player(&p).strip == STRIP_FAB_JUMP && pose_player(&p).frame == 1);
  p.vy = 0.0f;
  CHECK(pose_player(&p).frame == 2);
  p.vy = 300.0f;
  CHECK(pose_player(&p).frame == 3);
  p.vy = 800.0f;
  CHECK(pose_player(&p).frame == 4);
}

static void the_pistol_changes_how_he_stands_and_walks(void) {
  player_t p = standing();
  p.has_gun = true;
  CHECK(pose_player(&p).strip == STRIP_FAB_GUNAIM);
  p.vx = 100.0f;
  CHECK(pose_player(&p).strip == STRIP_FAB_GUNWALK);
  p.shoot_pose = 0.19f;
  CHECK(pose_player(&p).strip == STRIP_FAB_GUNSHOOT);
  p.shoot_pose = 0.05f;
  CHECK(pose_player(&p).strip == STRIP_FAB_GUNAIM);
}

static void actions_override_movement_poses(void) {
  player_t p = standing();
  p.melee_timer = 0.3f;
  CHECK(pose_player(&p).strip == STRIP_FAB_ATTACK && pose_player(&p).frame == 0);
  p.melee_timer = 0.02f;
  CHECK(pose_player(&p).frame == 3);
  p.melee_timer = 0.0f;
  p.drink_timer = 0.5f;
  CHECK(pose_player(&p).strip == STRIP_FAB_DRINK);
  p.drink_timer = 0.0f;
  p.cast_pose = 0.2f;
  CHECK(pose_player(&p).strip == STRIP_FAB_SPELL);
  p.cast_pose = 0.0f;
  p.hurt_timer = 0.3f;
  CHECK(pose_player(&p).strip == STRIP_FAB_HURT);
  p.alive = false;
  CHECK(pose_player(&p).strip == STRIP_FAB_HURT && pose_player(&p).frame == 3);
}

static void every_pose_is_a_frame_that_exists(void) {
  player_t p = standing();
  for (int i = 0; i < 5000; i++) {
    p.vx = (float)(i % 400) - 100.0f;
    p.vy = (float)(i % 1500) - 700.0f;
    p.grounded = (i % 3) != 0;
    p.has_gun = (i % 5) == 0;
    p.walk_phase = (float)i * 0.37f;
    p.anim_time = (float)i * 0.013f;
    p.melee_timer = (i % 7) == 0 ? 0.34f * (float)(i % 11) / 11.0f : 0.0f;
    p.shoot_pose = (i % 13) == 0 ? 0.1f : 0.0f;
    pose_t pose = pose_player(&p);
    CHECK(pose.strip >= 0 && pose.strip < STRIP_COUNT);
    int frames = kStripSpecs[pose.strip].frames;
    CHECK(pose.frame >= 0 && pose.frame < (frames ? frames : 1));
  }
}

static void zombies_shuffle_and_claw_when_close(void) {
  player_t p = standing();
  p.x = 100.0f;
  enemy_t z = {0};
  z.x = 300.0f;
  z.anim_time = 0.7f;
  CHECK(pose_zombie(&z, &p).strip == STRIP_ZOMBIE);
  z.x = 120.0f;
  CHECK(pose_zombie(&z, &p).strip == STRIP_ZOMBIE_ATTACK);
}

static void each_bug_and_effect_has_its_own_picture(void) {
  enemy_t b = {0};
  bool distinct[STRIP_COUNT] = {false};
  for (int v = 0; v < BUG_VARIANTS; v++) {
    b.variant = v;
    strip_id_t strip = pose_bug(&b).strip;
    CHECK(!distinct[strip]);
    distinct[strip] = true;
  }
  for (int k = FX_ZOMBIE_BOOM; k <= FX_BUG_SPLAT; k++)
    CHECK(pose_effect((fx_kind_t)k).strip >= STRIP_FX_MUZZLE);
}

void run_sprites_tests(void) {
  RUN(the_manifest_parser_reads_lines_and_rejects_junk);
  RUN(every_sprite_the_game_draws_was_cooked);
  RUN(cooked_sprites_have_sensible_sizes_and_anchors);
  RUN(a_standing_fabio_breathes_through_the_idle_frames);
  RUN(walking_and_running_cycle_the_swinging_run_frames);
  RUN(walking_visits_every_frame_so_the_legs_really_move);
  RUN(in_the_air_the_jump_frame_follows_the_climb_and_the_fall);
  RUN(the_pistol_changes_how_he_stands_and_walks);
  RUN(actions_override_movement_poses);
  RUN(every_pose_is_a_frame_that_exists);
  RUN(zombies_shuffle_and_claw_when_close);
  RUN(each_bug_and_effect_has_its_own_picture);
}
