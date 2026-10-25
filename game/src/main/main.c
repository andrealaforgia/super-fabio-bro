#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bot.h"
#include "buttons.h"
#include "command_line.h"
#include "controls.h"
#include "graphics.h"
#include "mixer.h"
#include "render.h"
#include "sdl_audio.h"
#include "sdl_keys.h"
#include "sfx.h"
#include "stepper.h"
#include "world.h"

#define SPRITE_DIR "assets_cooked/sprites"
#define FIXED_STEP (1.0 / 120.0)
#define VOICE_DELAY 0.7f

typedef struct {
  mixer_t mixer;
  music_t music;
  sample_t clips[SFX_COUNT];
  bool music_muted;
  bool sound_muted;
  float music_gain;
} audio_state_t;

typedef struct {
  const char* snapshot;
  float seconds;
  bool title;
  bool showcase;
  float delay;
  int seed;
  int width, height;
} options_t;

static void fill_audio(void* user, int16_t* out, int frames) {
  audio_state_t* audio = user;
  mixer_render(&audio->mixer, out, frames);
  if (!audio->music_muted) music_mix(&audio->music, out, frames, audio->music_gain);
}

static void play(audio_state_t* audio, sfx_id_t id, float volume) {
  if (audio->sound_muted) return;
  audio_lock();
  mixer_play(&audio->mixer, &audio->clips[id], volume);
  audio_unlock();
}

static void play_events(audio_state_t* audio, world_t* world) {
  for (int i = 0; i < world->event_count; i++) {
    float volume;
    sfx_id_t id = sfx_for_event(world->events[i].kind, &volume);
    play(audio, id, volume);
  }
  world_clear_events(world);
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
  world_t* world = malloc(sizeof(*world));
  world_init(world, (uint32_t)o->seed);
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
    world_kill_enemy(world, &world->enemies[6], false);
    for (int i = 0; i < 6; i++) world_update(world, &controls, 1.0f / 120.0f);
    render_world(gfx, look, world);
    bool ok = save_frame_bmp(gfx, o->snapshot);
    present_frame(gfx);
    free(world);
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
  free(world);
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

  static audio_state_t audio;
  mixer_init(&audio.mixer);
  audio.music_gain = 0.5f;
  bool sound = sfx_build(audio.clips) && audio_open(fill_audio, &audio);
  if (!sound) fprintf(stderr, "no sound: %s\n", SDL_GetError());

  world_t* world = malloc(sizeof(*world));
  world_init(world, (uint32_t)options.seed);
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
    if (controls_pressed(&controls, BTN_MUTE)) audio.music_muted = !audio.music_muted;
    if (controls_pressed(&controls, BTN_SOUND)) audio.sound_muted = !audio.sound_muted;
    bool start = controls_pressed(&controls, BTN_JUMP) || controls_pressed(&controls, BTN_CONFIRM);

    begin_frame(gfx, 0, 0, 0);
    world->view_w = gfx->view_width;

    if (delay_left > 0.0f) {
      delay_left -= (float)frame;
      audio.music_gain = 0.0f;
      render_countdown(gfx, &look, delay_left);
      present_frame(gfx);
      continue;
    }
    if (on_title) {
      title_time += (float)frame;
      /* Keep the music out of the way of the voice. */
      audio.music_gain = (title_time > VOICE_DELAY && title_time < VOICE_DELAY + 2.0f) ? 0.12f : 0.5f;
      if (sound && !voiced && title_time > VOICE_DELAY) {
        play(&audio, SFX_VOICE_FABIO, 1.0f);
        voiced = true;
      }
      if (start && title_time > 0.3f) {
        on_title = false;
        audio.music_gain = 0.5f;
        world_init(world, (uint32_t)options.seed);
        world->view_w = gfx->view_width;
        play(&audio, SFX_PICKUP, 0.6f);
      }
      render_title(gfx, &look, title_time);
    } else {
      bool over = world->state == STATE_GAME_OVER || world->state == STATE_WON;
      if (controls_pressed(&controls, BTN_PAUSE) && !over) paused = !paused;
      if (over && start && world->state_time > 1.0f) {
        options.seed = (int)(((uint32_t)time(NULL) + (uint32_t)SDL_GetTicks()) & 0x7fffffffu);
        world_init(world, (uint32_t)options.seed);
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
        if (sound) play_events(&audio, world);
        else world_clear_events(world);
      }
      render_world(gfx, &look, world);
      if (paused) render_pause(gfx, &look);
    }
    present_frame(gfx);
  }

  audio_close();
  if (sound) sfx_free(audio.clips);
  look_free(&look);
  terminate_graphics_context(&graphics);
  free(world);
  IMG_Quit();
  SDL_Quit();
  return 0;
}
