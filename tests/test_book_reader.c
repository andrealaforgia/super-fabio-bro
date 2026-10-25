#include <string.h>

#include "bot.h"
#include "pose.h"
#include "test_groups.h"
#include "test_support.h"
#include "test_world.h"

static world_t* fresh(void) {
  world_t* world = new_empty_world();
  settle(world);
  return world;
}

static int live_effects(const world_t* world, fx_kind_t kind) {
  int n = 0;
  size_t cursor = 0;
  for (const effect_t* fx; (fx = pool_next_active(&world->effects, &cursor));)
    n += fx->kind == kind;
  return n;
}

static void the_book_reader_turns_up_by_himself_off_screen(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 2.0f;
  world->player.invulnerable = 1e9f;
  step(world, NULL, 1.5f);
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 0);
  step(world, NULL, 1.0f);
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 1);
  size_t cursor = 0;
  const enemy_t* m = pool_next_active(&world->enemies, &cursor);
  CHECK(m != NULL && m->kind == ENEMY_BOOK_READER);
  CHECK(m->x < world->camera_x || m->x > world->camera_x + world->view_w);
  world_free(world);
}

static void there_is_only_one_book_reader_at_a_time_and_he_comes_back_later(void) {
  world_t* world = fresh();
  world->spawning = true;
  world->zombie_timer = 1e9f;
  world->bug_timer = 1e9f;
  world->book_reader_timer = 0.1f;
  world->player.invulnerable = 1e9f;
  int most = 0;
  for (int i = 0; i < 100; i++) {
    step(world, NULL, 1.0f);
    int n = world_count_enemies(world, ENEMY_BOOK_READER);
    if (n > most) most = n;
    world->player.x = fminf(world->player.x, 40.0f * TILE);
  }
  CHECK(most == 1);
  world_free(world);
}

static void walking_into_him_does_no_harm(void) {
  world_t* world = fresh();
  spawn_book_reader(world, world->player.x, world->player.y);
  step(world, NULL, 2.0f);
  CHECK(world->player.hp == PLAYER_HP);
  CHECK(world->player.hurt_timer <= 0.0f);
  CHECK(world->player.invulnerable <= 0.0f);
  CHECK(!world_has_event(world, EVENT_HURT));
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 1);
  world_free(world);
}

static void landing_on_him_or_swinging_the_pickaxe_does_not_set_him_off(void) {
  world_t* world = fresh();
  spawn_book_reader(world, world->player.x + 20.0f, world->player.y);
  world->player.y -= 30.0f;
  world->player.vy = 400.0f;
  step(world, NULL, 0.1f);
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 1);
  buttons_t swing = BTN_MELEE;
  step(world, &swing, 0.4f);
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 1);
  CHECK(world->book_readers_killed == 0);
  world_free(world);
}

static void a_bullet_makes_him_burst_and_scores(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  world->player.ammo = PISTOL_AMMO;
  spawn_book_reader(world, world->player.x + 250.0f, world->player.y);
  enemy_at(world, 0)->speed = 0.0f;
  buttons_t fire = BTN_SHOOT;
  step(world, &fire, 0.02f);
  step(world, NULL, 0.6f);
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 0);
  CHECK(world->book_readers_killed == 1);
  CHECK(world->score == 500);
  CHECK(world->zombies_killed == 0 && world->bugs_killed == 0);
  CHECK(pool_get_active_count(&world->bullets) == 0);
  CHECK(live_effects(world, FX_BOOK_READER_BOOM) == 1);
  CHECK(world_has_event(world, EVENT_BOOK_READER_DIES));
  world_free(world);
}

static void the_tdd_ray_makes_him_burst_too(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  world->player.spell_time = 10.0f;
  spawn_book_reader(world, world->player.x + 200.0f, world->player.y);
  buttons_t cast = BTN_SHOOT;
  step(world, &cast, 0.02f);
  CHECK(world->book_readers_killed == 1);
  CHECK(live_effects(world, FX_BOOK_READER_BOOM) == 1);
  world_free(world);
}

static void the_burst_plays_out_and_then_is_gone(void) {
  world_t* world = fresh();
  spawn_book_reader(world, world->player.x + 100.0f, world->player.y);
  world_kill_enemy(world, enemy_at(world, 0), false);
  CHECK(live_effects(world, FX_BOOK_READER_BOOM) == 1);
  step(world, NULL, 1.0f);
  CHECK(live_effects(world, FX_BOOK_READER_BOOM) == 1);
  step(world, NULL, 1.0f);
  CHECK(live_effects(world, FX_BOOK_READER_BOOM) == 0);
  world_free(world);
}

/* Runs the book reader for `seconds` and returns his moods, one per tenth of a
 * second, collapsed into runs: reading, laughing, reading, ... */
static int mood_runs(world_t* world, float seconds, book_reader_mood_t runs[16],
                     float lengths[16]) {
  int count = 0;
  for (float t = 0.0f; t < seconds; t += 0.1f) {
    step(world, NULL, 0.1f);
    book_reader_mood_t now = book_reader_mood(enemy_at(world, 0));
    if (count == 0 || runs[count - 1] != now) {
      if (count == 16) break;
      runs[count] = now;
      lengths[count++] = 0.0f;
    }
    lengths[count - 1] += 0.1f;
  }
  return count;
}

static void he_reads_then_laughs_then_reads_again_and_so_on(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_book_reader(world, world->player.x + 700.0f, world->player.y);
  book_reader_mood_t runs[16];
  float lengths[16];
  int count = mood_runs(world, 3.0f * (BOOK_READER_READ_SECONDS + BOOK_READER_LAUGH_SECONDS),
                        runs, lengths);
  CHECK(count >= 5);
  CHECK(runs[0] == BOOK_READER_READING);
  for (int i = 1; i < count; i++) CHECK(runs[i] != runs[i - 1]);
  CHECK(runs[1] == BOOK_READER_LAUGHING && runs[2] == BOOK_READER_READING);
  CHECK(fabsf(lengths[1] - BOOK_READER_LAUGH_SECONDS) < 0.25f);
  CHECK(fabsf(lengths[2] - BOOK_READER_READ_SECONDS) < 0.25f);
  world_free(world);
}

static void the_loop_does_not_depend_on_where_fabio_is(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_book_reader(world, world->player.x + 150.0f, world->player.y);
  book_reader_mood_t runs[16];
  float lengths[16];
  int count = mood_runs(world, 2.0f * (BOOK_READER_READ_SECONDS + BOOK_READER_LAUGH_SECONDS),
                        runs, lengths);
  CHECK(count >= 4);
  CHECK(runs[0] == BOOK_READER_READING && runs[1] == BOOK_READER_LAUGHING);
  world_free(world);
}

static void each_mood_shows_its_own_frames(void) {
  world_t* world = fresh();
  spawn_book_reader(world, world->player.x + 700.0f, world->player.y);
  enemy_t* m = enemy_at(world, 0);
  m->anim_time = 0.5f;
  CHECK(book_reader_mood(m) == BOOK_READER_READING);
  CHECK(pose_book_reader(m).strip == STRIP_READER_READ);
  m->anim_time = BOOK_READER_READ_SECONDS + 0.5f;
  CHECK(book_reader_mood(m) == BOOK_READER_LAUGHING);
  CHECK(pose_book_reader(m).strip == STRIP_READER_LAUGH);
  m->anim_time += BOOK_READER_LAUGH_SECONDS;
  CHECK(book_reader_mood(m) == BOOK_READER_READING);
  world_free(world);
}

static void he_ambles_while_reading_and_stands_still_while_laughing(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  spawn_book_reader(world, world->player.x + 700.0f, world->player.y);
  enemy_t* m = enemy_at(world, 0);
  float start = m->x;
  step(world, NULL, BOOK_READER_READ_SECONDS - 0.5f);
  CHECK(fabsf(m->x - start) > 10.0f);
  step(world, NULL, 0.7f);
  CHECK(book_reader_mood(m) == BOOK_READER_LAUGHING);
  float laughing_at = m->x;
  step(world, NULL, BOOK_READER_LAUGH_SECONDS - 0.4f);
  CHECK(fabsf(m->x - laughing_at) < 1.0f);
  world_free(world);
}

static void he_turns_back_at_a_drop_instead_of_falling_in(void) {
  world_t* world = fresh();
  world->player.invulnerable = 1000.0f;
  for (int col = 30; col < 40; col++)
    for (int row = GROUND_ROW; row < LEVEL_ROWS; row++)
      world->level.tiles[row][col] = TILE_EMPTY;
  world->player.x = 5.0f * TILE;
  spawn_book_reader(world, 28.5f * TILE, world->player.y);
  enemy_at(world, 0)->facing = 1;
  enemy_at(world, 0)->speed = 60.0f;
  step(world, NULL, 4.0f);
  CHECK(world_count_enemies(world, ENEMY_BOOK_READER) == 1);
  CHECK(enemy_at(world, 0)->x < 30.0f * TILE);
  world_free(world);
}

static void the_bot_does_not_waste_bullets_on_him(void) {
  world_t* world = fresh();
  world->player.ammo = PISTOL_AMMO;
  spawn_book_reader(world, world->player.x + 200.0f, world->player.y);
  bot_t bot;
  memset(&bot, 0, sizeof(bot));
  buttons_t now = bot_decide(world, &bot, 1.0f / 60.0f);
  CHECK((now & BTN_SHOOT) == 0);
  world_free(world);
}

static void his_burst_has_a_pose_for_every_moment(void) {
  CHECK(pose_book_reader_boom(0.0f).strip == STRIP_READER_POP);
  CHECK(pose_book_reader_boom(0.0f).frame == 0);
  CHECK(pose_book_reader_boom(0.5f).frame == 6);
  CHECK(pose_book_reader_boom(0.6f).strip == STRIP_READER_FLASH);
  CHECK(pose_book_reader_boom(0.8f).strip == STRIP_READER_BOOM);
  CHECK(pose_book_reader_boom(1.5f).strip == STRIP_READER_REMAINS);
}

void run_book_reader_tests(void) {
  RUN(the_book_reader_turns_up_by_himself_off_screen);
  RUN(there_is_only_one_book_reader_at_a_time_and_he_comes_back_later);
  RUN(walking_into_him_does_no_harm);
  RUN(landing_on_him_or_swinging_the_pickaxe_does_not_set_him_off);
  RUN(a_bullet_makes_him_burst_and_scores);
  RUN(the_tdd_ray_makes_him_burst_too);
  RUN(the_burst_plays_out_and_then_is_gone);
  RUN(he_reads_then_laughs_then_reads_again_and_so_on);
  RUN(the_loop_does_not_depend_on_where_fabio_is);
  RUN(each_mood_shows_its_own_frames);
  RUN(he_ambles_while_reading_and_stands_still_while_laughing);
  RUN(he_turns_back_at_a_drop_instead_of_falling_in);
  RUN(the_bot_does_not_waste_bullets_on_him);
  RUN(his_burst_has_a_pose_for_every_moment);
}
