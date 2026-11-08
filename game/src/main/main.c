#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bot.h"
#include "audio.h"
#include "buttons.h"
#include "clock.h"
#include "command_line.h"
#include "controls.h"
#include "engine.h"
#include "events.h"
#include "fps_tracker.h"
#include "graphics.h"
#include "logger.h"
#include "render.h"
#include "game_keys.h"
#include "sfx.h"
#include "stepper.h"
#include "world.h"

#define SPRITE_DIR "assets_cooked/sprites"
#define FIXED_STEP (1.0 / 120.0)
#define VOICE_DELAY 0.7f

typedef struct {
  const char* snapshot;
  float seconds;
  float start_x;
  bool title;
  bool showcase;
  float delay;
  int seed;
  int width, height;
} options_t;

static void music_source(void* user, int16_t* out, int frames, float gain) {
  music_mix(user, out, frames, gain);
}

/* Synthesises every sound effect and hands it to the engine's mixer. */
static bool install_sounds(audio_context_ptr audio) {
  sample_t clips[SFX_COUNT];
  if (!sfx_build(clips)) return false;
  bool ok = true;
  for (int i = 0; i < SFX_COUNT; i++)
    ok = set_sound(audio, i, clips[i].data, clips[i].length) && ok;
  sfx_free(clips);
  return ok;
}

/* The audio listens to the world: each event kind has its sound. */
static void play_event_sound(const game_event_t* event, void* user) {
  float volume;
  sfx_id_t id = sfx_for_event((event_kind_t)event->type, &volume);
  play_sound_at_volume(user, (int)id, volume);
  /* Fabio swears when he is hit. */
  if (event_makes_fabio_swear((event_kind_t)event->type)) play_sound_at_volume(user, SFX_VOICE_CURSE, 1.0f);
}

/* Lets a bot play for a while, then saves what the screen shows. */
static int take_snapshot(graphics_context_ptr gfx, const look_t* look, const options_t* o) {
  if (o->title) {
    begin_frame(gfx, 0, 0, 0);
    render_title(gfx, look, 1.2f);
    bool ok = save_frame_bmp(gfx, o->snapshot);
    present_frame(gfx);
    return ok ? 0 : 1;
  }
  world_t* world = world_new((uint32_t)o->seed, NULL);
  if (!world) return 1;
  begin_frame(gfx, 0, 0, 0);
  world->view_w = graphics_view_width(gfx);
  bot_t bot;
  memset(&bot, 0, sizeof(bot));
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  if (o->showcase) {
    /* A staged fight: zombies and bugs closing in, the spell blasting two. */
    player_t* p = &world->player;
    world->spawning = false;
    p->x += 300.0f;
    p->ammo = PISTOL_AMMO;
    p->spell_time = 9.0f;
    world->camera_x = p->x - world->view_w * 0.42f;
    spawn_zombie(world, p->x - 150.0f, p->y);
    spawn_zombie(world, p->x - 260.0f, p->y);
    spawn_bug(world, BUG_FLY, p->x - 70.0f, p->y - 120.0f);
    spawn_bug(world, BUG_BEETLE, p->x - 90.0f, p->y);
    spawn_zombie(world, p->x + 170.0f, p->y);
    spawn_bug(world, BUG_BEE, p->x + 120.0f, p->y - 90.0f);
    controls_update(&controls, BTN_SHOOT);
    for (int i = 0; i < 100; i++) world_update(world, &controls, 1.0f / 120.0f);
    spawn_zombie(world, p->x + 250.0f, p->y);
    world_kill_enemy(world, pool_get_at(&world->enemies, 6), false);
    spawn_book_reader(world, p->x + 330.0f, p->y);
    spawn_monk(world, MONK_HOODED, p->x - 415.0f, p->y);
    spawn_monk(world, MONK_SKELETON, p->x - 340.0f, p->y);
    spawn_monk(world, MONK_HOODED, p->x + 470.0f, p->y);
    spawn_monk(world, MONK_SKELETON, p->x + 520.0f, p->y);
    spawn_chef(world, p->x + 560.0f, p->y); /* far: smiling and pointing */
    size_t cursor = 0; /* the farther monk goes up in smoke */
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
      if (e->kind == ENEMY_MONK && e->x > p->x + 450.0f) world_kill_enemy(world, e, false);
    /* The gorilla has just let go of a banana, and it spins towards Fabio. */
    spawn_gorilla(world, p->x + 395.0f, p->y);
    cursor = 0;
    for (enemy_t* e; (e = pool_next_active(&world->enemies, &cursor));)
      if (e->kind == ENEMY_GORILLA) e->anim_time = GORILLA_CYCLE - GORILLA_THROW_SECONDS + 0.38f;
    for (int i = 0; i < 40; i++) world_update(world, &controls, 1.0f / 120.0f);
    render_world(gfx, look, world);
    bool ok = save_frame_bmp(gfx, o->snapshot);
    present_frame(gfx);
    world_free(world);
    return ok ? 0 : 1;
  }
  if (o->start_x > 0.0f) { /* a quiet look at one stretch of the level */
    world->spawning = false;
    world->player.x = o->start_x;
    world->camera_x = o->start_x - world->view_w * 0.42f;
  }
  for (float t = 0.0f; t < o->seconds; t += 1.0f / 120.0f) {
    buttons_t now = bot_decide(world, &bot, 1.0f / 120.0f);
    controls_update(&controls, now);
    world_update(world, &controls, 1.0f / 120.0f);
  }
  render_world(gfx, look, world);
  bool ok = save_frame_bmp(gfx, o->snapshot);
  present_frame(gfx);
  world_free(world);
  return ok ? 0 : 1;
}

int main(int argc, char** argv) {
  options_t options = {NULL, 4.0f, 0.0f, false, false, 0.0f,
                       (int)((uint32_t)time(NULL) & 0x7fffffffu), 0, 0};
  const custom_option_t custom[] = {
      {"snapshot", OPTION_STRING, &options.snapshot,
       "save a frame to this BMP file and quit"},
      {"seconds", OPTION_FLOAT, &options.seconds,
       "with --snapshot, how long a bot plays first (default 4)"},
      {"start-x", OPTION_FLOAT, &options.start_x,
       "with --snapshot, start Fabio here, with nothing spawning"},
      {"title", OPTION_FLAG, &options.title, "with --snapshot, the title screen"},
      {"showcase", OPTION_FLAG, &options.showcase,
       "with --snapshot, a staged fight"},
      {"seed", OPTION_INT, &options.seed, "the level's random seed"},
      {"width", OPTION_INT, &options.width, "window width (default: the screen)"},
      {"height", OPTION_INT, &options.height,
       "window height (default: the screen)"},
      {"delay", OPTION_FLOAT, &options.delay,
       "wait this many seconds behind a countdown, to start a screen recording"},
  };
  const size_t custom_count = sizeof(custom) / sizeof(custom[0]);
  command_line_options_t engine = default_command_line_options();
  engine.window_mode = WINDOWED;
  engine.vsync = true;
  engine.volume = 128;
  /* The engine options this game acts on; it offers no others. */
  const unsigned offered = CLI_WINDOW | CLI_VOLUME | CLI_GRAPHICS_INFO | CLI_SHOW_FPS;
  command_line_result_t parsed =
      parse_command_line(argc, argv, offered, &engine, custom, custom_count);
  if (parsed.status == COMMAND_LINE_ERROR) {
    fprintf(stderr, "%s\n", parsed.message);
    return 2;
  }
  if (parsed.status == COMMAND_LINE_HELP) {
    printf("usage: super-fabio-bros [options]\n");
    print_help(offered, &engine, custom, custom_count);
    return 0;
  }
  if (!engine_start()) {
    fprintf(stderr, "cannot start the engine: %s\n", engine_last_error());
    return 1;
  }
  if (engine.graphics_info) {
    print_graphics_info();
    engine_stop();
    return 0;
  }
  engine_set_log_level(LOG_LEVEL_WARN); /* the engine logs each texture */
  graphics_config_t config = default_graphics_config("Super Fabio Bros");
  config.width = options.width;
  config.height = options.height;
  config.display = engine.display;
  config.display_mode = engine.display_mode;
  config.window_mode = engine.window_mode;
  config.vsync = engine.vsync;
  config.view_height = VIEW_HEIGHT;
  graphics_context_ptr gfx = create_graphics_context(&config);
  if (!gfx) {
    fprintf(stderr, "cannot open a window: %s\n", engine_last_error());
    engine_stop();
    return 1;
  }
  look_t look;
  if (!look_load(&look, gfx, SPRITE_DIR)) {
    look_free(&look);
    terminate_graphics_context(gfx);
    engine_stop();
    return 1;
  }

  if (options.snapshot) {
    int status = take_snapshot(gfx, &look, &options);
    look_free(&look);
    terminate_graphics_context(gfx);
    engine_stop();
    return status;
  }

  static music_t music;
  audio_context_ptr audio = init_audio_context(SFX_COUNT, engine.volume);
  bool sound = audio && install_sounds(audio);
  if (!sound) fprintf(stderr, "no sound\n");
  bool music_on = true, sound_on = true;
  if (audio) {
    set_music_source(audio, music_source, &music);
    set_music_gain(audio, 0.5f);
  }

  static event_system_t events;
  events = create_event_system();
  if (sound)
    for (int kind = 0; kind < EVENT_KIND_COUNT; kind++)
      subscribe(&events, kind, play_event_sound, audio);
  world_t* world = world_new((uint32_t)options.seed, &events);
  if (!world) {
    fprintf(stderr, "out of memory\n");
    terminate_audio_context(audio);
    look_free(&look);
    terminate_graphics_context(gfx);
    engine_stop();
    return 1;
  }
  float delay_left = options.delay; /* time to start a screen recording */
  bool on_title = true, voiced = false, paused = false, running = true;
  float title_time = 0.0f;
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  double last = get_clock_seconds();
  fps_tracker_t fps = create_fps_tracker();

  while (running) {
    if (drain_events() == QUIT_EVENT) running = false;
    double now = get_clock_seconds();
    double frame = now - last;
    last = now;
    fps_tracker_frame(&fps, now);

    buttons_t buttons = read_buttons();
    controls_update(&controls, buttons);
    if (controls_pressed(&controls, BTN_FULLSCREEN)) toggle_fullscreen(gfx);
    if (audio && controls_pressed(&controls, BTN_MUTE)) {
      music_on = !music_on;
      set_music_enabled(audio, music_on);
    }
    if (audio && controls_pressed(&controls, BTN_SOUND)) {
      sound_on = !sound_on;
      set_sound_enabled(audio, sound_on);
    }
    bool start = controls_pressed(&controls, BTN_JUMP) || controls_pressed(&controls, BTN_CONFIRM);

    begin_frame(gfx, 0, 0, 0);
    world->view_w = graphics_view_width(gfx);

    if (delay_left > 0.0f) {
      delay_left -= (float)frame;
      if (audio) set_music_gain(audio, 0.0f);
      render_countdown(gfx, &look, delay_left);
      if (engine.show_fps) render_fps(gfx, &look, fps_tracker_current(&fps));
      present_frame(gfx);
      continue;
    }
    if (on_title) {
      title_time += (float)frame;
      /* Keep the music out of the way of the voice. */
      if (audio) set_music_gain(audio, (title_time > VOICE_DELAY && title_time < VOICE_DELAY + 2.0f) ? 0.12f : 0.5f);
      if (sound && !voiced && title_time > VOICE_DELAY) {
        play_sound_at_volume(audio, SFX_VOICE_FABIO, 1.0f);
        voiced = true;
      }
      if (start && title_time > 0.3f) {
        on_title = false;
        if (audio) set_music_gain(audio, 0.5f);
        world_restart(world, (uint32_t)options.seed);
        world->view_w = graphics_view_width(gfx);
        play_sound_at_volume(audio, SFX_PICKUP, 0.6f);
      }
      render_title(gfx, &look, title_time);
    } else {
      bool over = world->state == STATE_GAME_OVER || world->state == STATE_WON;
      if (controls_pressed(&controls, BTN_PAUSE) && !over) paused = !paused;
      if (over && start && world->state_time > 1.0f) {
        options.seed = (int)(((uint32_t)time(NULL) + (uint32_t)get_clock_ticks_ms()) & 0x7fffffffu);
        world_restart(world, (uint32_t)options.seed);
        world->view_w = graphics_view_width(gfx);
      } else if (!paused) {
        double step;
        int steps = stepper_plan(frame, FIXED_STEP, 0.1, &step);
        controls_t substep = controls;
        for (int i = 0; i < steps; i++) {
          world_update(world, &substep, (float)step);
          /* A press counts for the first step only. */
          controls_consume_presses(&substep);
        }
      }
      render_world(gfx, &look, world);
      if (paused) render_pause(gfx, &look);
    }
    if (engine.show_fps) render_fps(gfx, &look, fps_tracker_current(&fps));
    present_frame(gfx);
  }

  terminate_audio_context(audio);
  look_free(&look);
  terminate_graphics_context(gfx);
  world_free(world);
  engine_stop();
  return 0;
}
