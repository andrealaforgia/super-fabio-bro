#include "sfx.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdlib.h>

#include "rng.h"

#define TAU 6.28318530718

static double square(double phase, double duty) {
  return fmod(phase, 1.0) < duty ? 1.0 : -1.0;
}

static double saw(double phase) { return 2.0 * fmod(phase, 1.0) - 1.0; }

static double triangle(double phase) {
  double p = fmod(phase, 1.0);
  return 4.0 * fabs(p - 0.5) - 1.0;
}

typedef double (*voice_fn)(double t, double duration, rng_t* rng, void* state);

static bool render(sample_t* clip, double seconds, voice_fn voice, uint32_t seed) {
  int length = (int)(seconds * MIXER_RATE);
  int16_t* data = malloc((size_t)length * sizeof(int16_t));
  if (!data) return false;
  rng_t rng;
  rng_seed(&rng, seed);
  double state[4] = {0, 0, 0, 0};
  for (int i = 0; i < length; i++) {
    double t = (double)i / MIXER_RATE;
    double v = voice(t, seconds, &rng, state);
    /* A short fade at the end keeps clips from clicking. */
    double tail = (seconds - t) * 200.0;
    if (tail < 1.0) v *= tail;
    if (v > 1.0) v = 1.0;
    if (v < -1.0) v = -1.0;
    data[i] = (int16_t)(v * 26000.0);
  }
  clip->data = data;
  clip->length = length;
  return true;
}

static double noise(rng_t* rng) { return rng_float(rng) * 2.0 - 1.0; }

static double decay(double t, double rate) { return exp(-t * rate); }

static double jump_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 280.0 + 700.0 * t / 0.18;
  return 0.35 * square(f * t, 0.5) * decay(t, 7.0);
}

static double shoot_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.35 * (noise(r) - lp[0]);
  return (0.8 * lp[0] + 0.5 * sin(TAU * (160.0 - 300.0 * t) * t)) * decay(t, 18.0);
}

static double spell_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 500.0 + 1500.0 * t / 0.3 + 60.0 * sin(TAU * 30.0 * t);
  return 0.3 * (sin(TAU * f * t) + 0.5 * sin(TAU * f * 1.5 * t)) *
         decay(t, 5.0);
}

static double melee_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.12 * (noise(r) - lp[0]);
  return 1.6 * lp[0] * sin(3.14159 * t / 0.16);
}

static double zombie_boom_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.18 * (noise(r) - lp[0]);
  return (1.8 * lp[0] + 0.6 * sin(TAU * (90.0 - 60.0 * t) * t)) * decay(t, 6.0);
}

static double bug_boom_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.5 * (noise(r) - lp[0]);
  return (1.1 * lp[0] + 0.3 * square(300.0 * t * (1.0 - t), 0.3)) * decay(t, 12.0);
}

/* A big, rumbling pop with a rain of paper after it. */
static double book_reader_boom_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.12 * (noise(r) - lp[0]);
  double thump = sin(TAU * (70.0 - 40.0 * t) * t) * decay(t, 4.0);
  double rumble = 2.2 * lp[0] * decay(t, 3.2);
  double paper = t > 0.35 ? 0.25 * noise(r) * decay(t - 0.35, 9.0) : 0.0;
  return thump + rumble + paper;
}

static double coin_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = t < 0.07 ? 988.0 : 1319.0;
  return 0.3 * square(f * t, 0.5) * decay(fmax(0.0, t - 0.07), 6.0) *
         (t < 0.07 ? 1.0 : 1.0);
}

static double pickup_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  static const double notes[] = {523.25, 659.25, 783.99, 1046.5};
  int n = (int)(t / 0.07);
  if (n > 3) n = 3;
  return 0.3 * square(notes[n] * t, 0.25) * decay(t - n * 0.07, 4.0);
}

static double drink_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  /* Three gulps, then a bright sparkle. */
  double gulp = 0.0;
  for (int i = 0; i < 3; i++) {
    double local = t - i * 0.14;
    if (local >= 0.0 && local < 0.1)
      gulp += 0.5 * sin(TAU * (180.0 + 260.0 * local / 0.1) * local) *
              sin(3.14159 * local / 0.1);
  }
  double sparkle = 0.0;
  if (t > 0.45) {
    double local = t - 0.45;
    sparkle = 0.25 * sin(TAU * (900.0 + 2400.0 * local) * local) * decay(local, 4.0);
  }
  return gulp + sparkle;
}

static double hurt_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 440.0 - 300.0 * t / 0.3;
  return 0.4 * saw(f * t) * decay(t, 4.0);
}

static double bump_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  return 0.6 * sin(TAU * (130.0 - 60.0 * t / 0.1) * t) * decay(t, 25.0);
}

static double stomp_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  return 0.5 * square((420.0 - 800.0 * t) * t, 0.5) * decay(t, 14.0);
}

static double block_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 330.0 + 520.0 * (t > 0.08 ? 1.0 : 0.0);
  return 0.3 * triangle(f * t) * decay(fmod(t, 0.08), 6.0);
}

static double death_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  static const double notes[] = {392.0, 370.0, 349.0, 330.0, 293.0, 262.0};
  int n = (int)(t / 0.18);
  if (n > 5) n = 5;
  return 0.35 * square(notes[n] * t, 0.5) * decay(t - n * 0.18, 2.5);
}

static double win_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  static const double notes[] = {523.25, 659.25, 783.99, 1046.5, 783.99, 1046.5};
  static const double starts[] = {0.0, 0.14, 0.28, 0.42, 0.7, 0.84};
  int n = 0;
  for (int i = 0; i < 6; i++)
    if (t >= starts[i]) n = i;
  return 0.3 * (square(notes[n] * t, 0.25) + 0.5 * triangle(notes[n] * 0.5 * t)) *
         decay(t - starts[n], 2.0);
}

static double heal_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 600.0 + 500.0 * t / 0.3;
  return 0.3 * triangle(f * t) * sin(3.14159 * t / 0.3);
}


/* "It's me! Fabio!" by formant synthesis: a buzzing or hissing source
 * through three resonators whose centres glide from phoneme to phoneme. */
typedef struct {
  double f1, f2, f3; /* formants, Hz */
  double duration;
  double pitch0, pitch1; /* 0 for unvoiced */
  double hiss;           /* 0..1 noise in the source */
  double level;
} phoneme_t;

static const phoneme_t kSpeech[] = {
    {400, 1900, 2500, 0.10, 165, 175, 0.0, 1.0},   /* I  */
    {3000, 4200, 5200, 0.03, 0, 0, 1.0, 0.7},      /* t  */
    {4500, 5500, 6500, 0.11, 0, 0, 1.0, 0.8},      /* s  */
    {250, 1200, 2200, 0.08, 180, 190, 0.0, 0.5},   /* m  */
    {290, 2250, 2900, 0.26, 200, 150, 0.0, 1.0},   /* ee */
    {300, 1000, 2200, 0.16, 0, 0, 0.0, 0.0},       /* pause */
    {1500, 2500, 4000, 0.07, 0, 0, 1.0, 0.45},     /* f  */
    {800, 1250, 2500, 0.17, 175, 195, 0.0, 1.0},   /* a  */
    {250, 1100, 2200, 0.07, 190, 190, 0.0, 0.45},  /* b  */
    {290, 2250, 2900, 0.11, 190, 185, 0.0, 0.95},  /* ee */
    {500, 900, 2500, 0.34, 205, 125, 0.0, 1.0},    /* o  */
};

static double speak(const phoneme_t* table, int count, double t, rng_t* r) {
  static double y1[3], y2[3], f[3], phase;
  if (t == 0.0) {
    for (int k = 0; k < 3; k++) y1[k] = y2[k] = 0.0, f[k] = 500.0 + 500.0 * k;
    phase = 0.0;
  }
  double start = 0.0;
  const phoneme_t* now = NULL;
  double local = 0.0;
  for (int i = 0; i < count; i++) {
    if (t < start + table[i].duration) {
      now = &table[i];
      local = (t - start) / now->duration;
      break;
    }
    start += table[i].duration;
  }
  if (!now) return 0.0;
  double target[3] = {now->f1, now->f2, now->f3};
  double pitch = now->pitch0 + (now->pitch1 - now->pitch0) * local;
  phase += pitch / MIXER_RATE;
  double voiced = pitch > 0.0 ? saw(phase) * (1.0 - now->hiss) : 0.0;
  double source = voiced + noise(r) * now->hiss;
  double envelope = fmin(1.0, sin(3.14159 * local) * 3.0);
  double out = 0.0;
  static const double weight[3] = {1.0, 0.7, 0.4};
  static const double bandwidth[3] = {70.0, 110.0, 180.0};
  for (int k = 0; k < 3; k++) {
    f[k] += (target[k] - f[k]) * 0.006;
    double rr = exp(-3.14159 * bandwidth[k] * (now->hiss > 0.9 ? 6.0 : 1.0) / MIXER_RATE);
    double a1 = 2.0 * rr * cos(TAU * f[k] / MIXER_RATE), a2 = -rr * rr;
    double y = (1.0 - rr) * source + a1 * y1[k] + a2 * y2[k];
    y2[k] = y1[k];
    y1[k] = y;
    out += weight[k] * y;
  }
  return out * now->level * envelope * 0.2;
}

static double speech_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)s;
  return speak(kSpeech, (int)(sizeof(kSpeech) / sizeof(kSpeech[0])), t, r);
}

/* "I don't test..." in a low, rasping, dragging zombie voice. */
static const phoneme_t kZombie[] = {
    {800, 1250, 2500, 0.15, 98, 104, 0.25, 1.0}, /* a of I */
    {300, 2200, 2900, 0.10, 104, 96, 0.25, 0.9}, /* ee of I */
    {250, 1100, 2200, 0.05, 90, 90, 0.0, 0.4},   /* d */
    {500, 900, 2500, 0.22, 96, 88, 0.25, 1.0},   /* o */
    {250, 1300, 2200, 0.10, 90, 88, 0.1, 0.6},   /* n */
    {3000, 4200, 5200, 0.04, 0, 0, 1.0, 0.7},    /* t */
    {530, 1840, 2480, 0.15, 96, 92, 0.25, 1.0},  /* e */
    {4500, 5500, 6500, 0.12, 0, 0, 1.0, 0.8},    /* s */
    {3000, 4200, 5200, 0.05, 0, 0, 1.0, 0.7},    /* t */
    {400, 1000, 2400, 0.08, 0, 0, 0.0, 0.0},     /* pause */
    {600, 1100, 2400, 0.50, 90, 55, 0.35, 0.7},  /* ...uhh */
};

static double zombie_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)s;
  return speak(kZombie, (int)(sizeof(kZombie) / sizeof(kZombie[0])), t, r);
}

/* Brings a clip's loudest sample up to `peak`: speech comes out quiet. */
static void normalise(sample_t* clip, double peak) {
  int16_t* data = (int16_t*)clip->data;
  int loudest = 1;
  for (int i = 0; i < clip->length; i++)
    if (abs(data[i]) > loudest) loudest = abs(data[i]);
  double gain = peak / loudest;
  for (int i = 0; i < clip->length; i++) data[i] = (int16_t)(data[i] * gain);
}

bool sfx_build(sample_t clips[SFX_COUNT]) {
  for (int i = 0; i < SFX_COUNT; i++) clips[i] = (sample_t){NULL, 0};
  bool ok = true;
  ok = ok && render(&clips[SFX_JUMP], 0.22, jump_voice, 1);
  ok = ok && render(&clips[SFX_SHOOT], 0.22, shoot_voice, 2);
  ok = ok && render(&clips[SFX_SPELL], 0.45, spell_voice, 3);
  ok = ok && render(&clips[SFX_MELEE], 0.18, melee_voice, 4);
  ok = ok && render(&clips[SFX_ZOMBIE_BOOM], 0.6, zombie_boom_voice, 5);
  ok = ok && render(&clips[SFX_BUG_BOOM], 0.3, bug_boom_voice, 6);
  ok = ok && render(&clips[SFX_COIN], 0.35, coin_voice, 7);
  ok = ok && render(&clips[SFX_PICKUP], 0.4, pickup_voice, 8);
  ok = ok && render(&clips[SFX_DRINK], 1.0, drink_voice, 9);
  ok = ok && render(&clips[SFX_HURT], 0.4, hurt_voice, 10);
  ok = ok && render(&clips[SFX_BUMP], 0.15, bump_voice, 11);
  ok = ok && render(&clips[SFX_STOMP], 0.2, stomp_voice, 12);
  ok = ok && render(&clips[SFX_BLOCK], 0.3, block_voice, 13);
  ok = ok && render(&clips[SFX_GROAN], 1.7, zombie_voice, 14);
  ok = ok && render(&clips[SFX_DEATH], 1.2, death_voice, 15);
  ok = ok && render(&clips[SFX_WIN], 1.5, win_voice, 16);
  ok = ok && render(&clips[SFX_HEAL], 0.35, heal_voice, 17);
  ok = ok && render(&clips[SFX_VOICE_FABIO], 1.5, speech_voice, 18);
  ok = ok && render(&clips[SFX_BOOK_READER_BOOM], 1.1, book_reader_boom_voice, 19);
  if (ok) {
    normalise(&clips[SFX_VOICE_FABIO], 31000.0);
    normalise(&clips[SFX_GROAN], 28000.0);
  }
  if (!ok) sfx_free(clips);
  return ok;
}

void sfx_free(sample_t clips[SFX_COUNT]) {
  for (int i = 0; i < SFX_COUNT; i++) {
    free((void*)clips[i].data);
    clips[i] = (sample_t){NULL, 0};
  }
}

sfx_id_t sfx_for_event(event_kind_t kind, float* volume) {
  *volume = 0.7f;
  switch (kind) {
    case EVENT_JUMP: *volume = 0.45f; return SFX_JUMP;
    case EVENT_SHOOT: return SFX_SHOOT;
    case EVENT_SPELL: return SFX_SPELL;
    case EVENT_MELEE: *volume = 0.6f; return SFX_MELEE;
    case EVENT_ZOMBIE_DIES: *volume = 0.9f; return SFX_ZOMBIE_BOOM;
    case EVENT_BUG_DIES: return SFX_BUG_BOOM;
    case EVENT_COIN: *volume = 0.5f; return SFX_COIN;
    case EVENT_PICKUP: return SFX_PICKUP;
    case EVENT_DRINK: return SFX_DRINK;
    case EVENT_HURT: *volume = 0.9f; return SFX_HURT;
    case EVENT_BUMP: return SFX_BUMP;
    case EVENT_STOMP: return SFX_STOMP;
    case EVENT_BLOCK_OPENS: *volume = 0.5f; return SFX_BLOCK;
    case EVENT_GROAN: *volume = 0.6f; return SFX_GROAN;
    case EVENT_DEATH: return SFX_DEATH;
    case EVENT_WIN: return SFX_WIN;
    case EVENT_HEAL: return SFX_HEAL;
    case EVENT_BOOK_READER_DIES: *volume = 0.9f; return SFX_BOOK_READER_BOOM;
    case EVENT_KIND_COUNT: break;
  }
  return SFX_BUMP;
}

/* A small bouncy tune in C: melody in eighth notes (MIDI numbers, 0 is a
 * rest), and a bass in quarter notes. */
static const int kMelody[64] = {
    76, 79, 76, 72, 76, 79, 84, 79, 74, 77, 74, 71, 74, 77, 83, 77,
    72, 76, 79, 76, 72, 76, 79, 76, 69, 72, 76, 72, 67, 71, 74, 71,
    77, 81, 77, 72, 77, 81, 84, 81, 76, 79, 76, 72, 76, 79, 84, 79,
    74, 79, 83, 79, 74, 79, 83, 86, 84, 0,  79, 0,  76, 0,  72, 0};
static const int kBass[32] = {48, 55, 48, 55, 43, 50, 43, 50, 48, 55, 48,
                              55, 45, 52, 43, 50, 41, 48, 41, 48, 48, 55,
                              48, 55, 43, 50, 43, 47, 48, 0,  48, 0};
#define EIGHTH 0.118

double music_loop_seconds(void) { return 64.0 * EIGHTH; }

static double note_frequency(int midi) {
  return 440.0 * pow(2.0, (midi - 69) / 12.0);
}

void music_mix(music_t* music, int16_t* out, int frames, float volume) {
  for (int i = 0; i < frames; i++) {
    double t = music->time;
    int step = (int)(t / EIGHTH) % 64;
    double in_note = fmod(t, EIGHTH);
    double sample = 0.0;
    if (kMelody[step]) {
      double f = note_frequency(kMelody[step]);
      sample += 0.5 * square(f * t, 0.25) * exp(-in_note * 9.0);
    }
    int beat = (step / 2) % 32;
    double in_beat = fmod(t, 2.0 * EIGHTH);
    if (kBass[beat]) {
      double f = note_frequency(kBass[beat]);
      sample += 0.7 * triangle(f * t) * exp(-in_beat * 3.0);
    }
    int mixed = out[i] + (int)(sample * volume * 9000.0);
    out[i] = (int16_t)(mixed > 32767 ? 32767 : (mixed < -32768 ? -32768 : mixed));
    music->time += 1.0 / MIXER_RATE;
    if (music->time >= music_loop_seconds()) music->time -= music_loop_seconds();
  }
}
