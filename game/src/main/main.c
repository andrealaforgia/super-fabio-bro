#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bot.h"
#include "audio.h"
#include "buttons.h"
#include "command_line.h"
#include "controls.h"
#include "graphics.h"
#include "render.h"
#include "sdl_keys.h"
#include "sfx.h"
#include "stepper.h"
#include "world.h"

#define SPRITE_DIR "assets_cooked/sprites"
#define FIXED_STEP (1.0 / 120.0)
#define VOICE_DELAY 0.7f

typedef struct {
  const char* snapshot;
  float seconds;
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
  world->view_w = gfx->view_width;
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
    for (int i = 0; i < 6; i++) world_update(world, &controls, 1.0f / 120.0f);
    render_world(gfx, look, world);
    bool ok = save_frame_bmp(gfx, o->snapshot);
    present_frame(gfx);
    world_free(world);
    return ok ? 0 : 1;
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
  options_t options = {NULL, 4.0f, false, false, 0.0f,
                       (int)((uint32_t)time(NULL) & 0x7fffffffu), 0, 0};
  const custom_option_t custom[] = {
      {"snapshot", OPTION_STRING, &options.snapshot,
       "save a frame to this BMP file and quit"},
      {"seconds", OPTION_FLOAT, &options.seconds,
       "with --snapshot, how long a bot plays first (default 4)"},
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
  parse_command_line_options(argc, argv, &engine, custom, custom_count);
  if (engine.help) {
    printf("usage: super-fabio-bros [options]\n");
    print_help(&engine, custom, custom_count);
    return 0;
  }
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "SDL: %s\n", SDL_GetError());
    return 1;
  }
  IMG_Init(IMG_INIT_PNG);
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN); /* the engine logs each texture */
  graphics_config_t config = default_graphics_config("Super Fabio Bros");
  config.width = options.width;
  config.height = options.height;
  config.display = engine.display;
  config.display_mode = engine.display_mode;
  config.window_mode = engine.window_mode;
  config.vsync = engine.vsync;
  config.view_height = VIEW_HEIGHT;
  graphics_context_t graphics = initialize_graphics_context(&config);
  graphics_context_ptr gfx = &graphics;
  if (!graphics.window || !graphics.renderer) {
    fprintf(stderr, "cannot open a window: %s\n", SDL_GetError());
    return 1;
  }
  look_t look;
  if (!look_load(&look, gfx, SPRITE_DIR)) return 1;

  if (options.snapshot) {
    int status = take_snapshot(gfx, &look, &options);
    look_free(&look);
    terminate_graphics_context(&graphics);
    SDL_Quit();
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
  if (!world) return 1;
  float delay_left = options.delay; /* time to start a screen recording */
  bool on_title = true, voiced = false, paused = false, running = true;
  float title_time = 0.0f;
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  Uint64 last = SDL_GetPerformanceCounter();

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event))
      if (event.type == SDL_QUIT) running = false;
    Uint64 now = SDL_GetPerformanceCounter();
    double frame = (double)(now - last) / (double)SDL_GetPerformanceFrequency();
    last = now;

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
    world->view_w = gfx->view_width;

    if (delay_left > 0.0f) {
      delay_left -= (float)frame;
      if (audio) set_music_gain(audio, 0.0f);
      render_countdown(gfx, &look, delay_left);
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
        world->view_w = gfx->view_width;
        play_sound_at_volume(audio, SFX_PICKUP, 0.6f);
      }
      render_title(gfx, &look, title_time);
    } else {
      bool over = world->state == STATE_GAME_OVER || world->state == STATE_WON;
      if (controls_pressed(&controls, BTN_PAUSE) && !over) paused = !paused;
      if (over && start && world->state_time > 1.0f) {
        options.seed = (int)(((uint32_t)time(NULL) + (uint32_t)SDL_GetTicks()) & 0x7fffffffu);
        world_restart(world, (uint32_t)options.seed);
        world->view_w = gfx->view_width;
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
    present_frame(gfx);
  }

  terminate_audio_context(audio);
  look_free(&look);
  terminate_graphics_context(&graphics);
  world_free(world);
  IMG_Quit();
  SDL_Quit();
  return 0;
}
