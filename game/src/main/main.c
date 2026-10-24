#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bot.h"
#include "controls.h"
#include "demo.h"
#include "mixer.h"
#include "render.h"
#include "sdl_audio.h"
#include "sdl_gfx.h"
#include "sdl_keyboard.h"
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
  bool muted;
  float music_gain;
} audio_state_t;

typedef struct {
  const char* snapshot;
  float seconds;
  bool title;
  bool showcase;
  bool demo;
  uint32_t seed;
  int width, height;
} options_t;

static void fill_audio(void* user, int16_t* out, int frames) {
  audio_state_t* audio = user;
  mixer_render(&audio->mixer, out, frames);
  if (!audio->muted) music_mix(&audio->music, out, frames, audio->music_gain);
}

static void play(audio_state_t* audio, sfx_id_t id, float volume) {
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

static options_t parse(int argc, char** argv) {
  options_t o = {NULL, 4.0f, false, false, false, (uint32_t)time(NULL), 0, 0};
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--snapshot") && i + 1 < argc) o.snapshot = argv[++i];
    else if (!strcmp(argv[i], "--seconds") && i + 1 < argc) o.seconds = (float)atof(argv[++i]);
    else if (!strcmp(argv[i], "--seed") && i + 1 < argc) o.seed = (uint32_t)atol(argv[++i]);
    else if (!strcmp(argv[i], "--width") && i + 1 < argc) o.width = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--height") && i + 1 < argc) o.height = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--title")) o.title = true;
    else if (!strcmp(argv[i], "--showcase")) o.showcase = true;
    else if (!strcmp(argv[i], "--demo")) o.demo = true;
    else {
      printf("usage: super-fabio-bros [--seed N] [--snapshot out.bmp [--seconds S] [--demo] [--title|--showcase]"
             " [--width W --height H]]\n");
      exit(!strcmp(argv[i], "--help") ? 0 : 2);
    }
  }
  return o;
}

/* Lets a bot play for a while, then saves what the screen shows. */
static int take_snapshot(gfx_t* gfx, const look_t* look, const options_t* o) {
  if (o->title) {
    gfx_begin(gfx, 0, 0, 0);
    render_title(gfx, look, 1.2f);
    bool ok = gfx_save_bmp(gfx, o->snapshot);
    gfx_end(gfx);
    return ok ? 0 : 1;
  }
  world_t* world = malloc(sizeof(*world));
  demo_t demo;
  if (o->demo) demo_world_init(world, &demo, o->seed);
  else world_init(world, o->seed);
  gfx_begin(gfx, 0, 0, 0);
  world->view_w = gfx_view_width(gfx);
  if (o->demo) {
    controls_t pad;
    memset(&pad, 0, sizeof(pad));
    for (float t = 0.0f; t < o->seconds && !demo_finished(&demo); t += 1.0f / 120.0f) {
      buttons_t now = demo_decide(world, &demo, 1.0f / 120.0f);
      controls_update(&pad, &now);
      world_update(world, &pad, 1.0f / 120.0f);
      world_clear_events(world);
    }
    render_world(gfx, look, world);
    render_caption(gfx, demo_caption(&demo));
    bool ok = gfx_save_bmp(gfx, o->snapshot);
    gfx_end(gfx);
    free(world);
    return ok ? 0 : 1;
  }
  bot_t bot;
  memset(&bot, 0, sizeof(bot));
  controls_t controls;
  memset(&controls, 0, sizeof(controls));
  if (o->showcase) {
    /* A staged fight: zombies and bugs closing in, the spell blasting two. */
    player_t* p = &world->player;
    world->spawning = false;
    p->x += 300.0f;
    p->has_gun = true;
    p->spell_time = 9.0f;
    world->camera_x = p->x - world->view_w * 0.42f;
    spawn_zombie(world, p->x - 150.0f, p->y);
    spawn_zombie(world, p->x - 260.0f, p->y);
    spawn_bug(world, BUG_FLY, p->x - 70.0f, p->y - 120.0f);
    spawn_bug(world, BUG_BEETLE, p->x - 90.0f, p->y);
    spawn_zombie(world, p->x + 170.0f, p->y);
    spawn_bug(world, BUG_BEE, p->x + 120.0f, p->y - 90.0f);
    buttons_t fire;
    memset(&fire, 0, sizeof(fire));
    fire.shoot = true;
    controls_update(&controls, &fire);
    for (int i = 0; i < 100; i++) world_update(world, &controls, 1.0f / 120.0f);
    spawn_zombie(world, p->x + 250.0f, p->y);
    world_kill_enemy(world, &world->enemies[6], false);
    for (int i = 0; i < 6; i++) world_update(world, &controls, 1.0f / 120.0f);
    render_world(gfx, look, world);
    bool ok = gfx_save_bmp(gfx, o->snapshot);
    gfx_end(gfx);
    free(world);
    return ok ? 0 : 1;
  }
  for (float t = 0.0f; t < o->seconds; t += 1.0f / 120.0f) {
    buttons_t now = bot_decide(world, &bot, 1.0f / 120.0f);
    controls_update(&controls, &now);
    world_update(world, &controls, 1.0f / 120.0f);
  }
  render_world(gfx, look, world);
  bool ok = gfx_save_bmp(gfx, o->snapshot);
  gfx_end(gfx);
  free(world);
  return ok ? 0 : 1;
}

int main(int argc, char** argv) {
  options_t options = parse(argc, argv);
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "SDL: %s\n", SDL_GetError());
    return 1;
  }
  IMG_Init(IMG_INIT_PNG);
  gfx_t* gfx = gfx_open("Super Fabio Bros", options.width, options.height);
  if (!gfx) {
    fprintf(stderr, "cannot open a window: %s\n", SDL_GetError());
    return 1;
  }
  look_t look;
  if (!look_load(&look, gfx, SPRITE_DIR)) return 1;

  if (options.snapshot) {
    int status = take_snapshot(gfx, &look, &options);
    look_free(&look);
    gfx_close(gfx);
    SDL_Quit();
    return status;
  }

  static audio_state_t audio;
  mixer_init(&audio.mixer);
  audio.music_gain = 0.5f;
  bool sound = sfx_build(audio.clips) && audio_open(fill_audio, &audio);
  if (!sound) fprintf(stderr, "no sound: %s\n", SDL_GetError());

  world_t* world = malloc(sizeof(*world));
  world_init(world, options.seed);
  demo_t demo;
  controls_t demo_pad;
  memset(&demo_pad, 0, sizeof(demo_pad));
  float end_time = 0.0f;
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

    buttons_t buttons = keyboard_read();
    controls_update(&controls, &buttons);
    if (controls.pressed.fullscreen) gfx_toggle_fullscreen(gfx);
    if (controls.pressed.mute) audio.muted = !audio.muted;
    bool start = controls.pressed.jump || controls.pressed.confirm;

    gfx_begin(gfx, 0, 0, 0);
    world->view_w = gfx_view_width(gfx);

    if (on_title) {
      title_time += (float)frame;
      /* Keep the music out of the way of the voice. */
      audio.music_gain = (title_time > VOICE_DELAY && title_time < VOICE_DELAY + 2.0f) ? 0.12f : 0.5f;
      if (sound && !voiced && title_time > VOICE_DELAY) {
        play(&audio, SFX_VOICE_FABIO, 1.0f);
        voiced = true;
      }
      /* In the demo nobody touches the keyboard: space is "pressed" for us
       * once the voice has had its say. */
      if (options.demo) start = title_time > VOICE_DELAY + 3.0f;
      if (start && title_time > 0.3f) {
        on_title = false;
        audio.music_gain = 0.5f;
        if (options.demo) demo_world_init(world, &demo, options.seed);
        else world_init(world, options.seed);
        end_time = 0.0f;
        world->view_w = gfx_view_width(gfx);
        play(&audio, SFX_PICKUP, 0.6f);
      }
      render_title(gfx, &look, title_time);
    } else if (options.demo) {
      if (controls.pressed.pause) running = false;
      double step;
      int steps = stepper_plan(frame, FIXED_STEP, 0.1, &step);
      for (int i = 0; i < steps && !demo_finished(&demo); i++) {
        buttons_t now = demo_decide(world, &demo, (float)step);
        controls_update(&demo_pad, &now);
        world_update(world, &demo_pad, (float)step);
      }
      if (sound) play_events(&audio, world);
      else world_clear_events(world);
      render_world(gfx, &look, world);
      render_caption(gfx, demo_caption(&demo));
      if (demo_finished(&demo)) {
        end_time += (float)frame;
        if (end_time > 3.0f) {
          on_title = true;
          voiced = false;
          title_time = 0.0f;
        }
      }
    } else {
      bool over = world->state == STATE_GAME_OVER || world->state == STATE_WON;
      if (controls.pressed.pause && !over) paused = !paused;
      if (over && start && world->state_time > 1.0f) {
        options.seed = (uint32_t)time(NULL) + (uint32_t)SDL_GetTicks();
        world_init(world, options.seed);
        world->view_w = gfx_view_width(gfx);
      } else if (!paused) {
        double step;
        int steps = stepper_plan(frame, FIXED_STEP, 0.1, &step);
        controls_t substep = controls;
        for (int i = 0; i < steps; i++) {
          world_update(world, &substep, (float)step);
          /* A press counts for the first step only. */
          memset(&substep.pressed, 0, sizeof(substep.pressed));
        }
        if (sound) play_events(&audio, world);
        else world_clear_events(world);
      }
      render_world(gfx, &look, world);
      if (paused) render_pause(gfx);
    }
    gfx_end(gfx);
  }

  audio_close();
  if (sound) sfx_free(audio.clips);
  look_free(&look);
  gfx_close(gfx);
  free(world);
  IMG_Quit();
  SDL_Quit();
  return 0;
}
