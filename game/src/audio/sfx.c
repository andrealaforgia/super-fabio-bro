#include "sfx.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "rng.h"
#include "drum_sounds.h"
#include "voice_curse.h"
#include "voice_slop.h"

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

/* A soft bloop, rising: a bubble leaving the nozzle. */
static double bubble_shot_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  return 0.45 * sin(TAU * (260.0 + 900.0 * t) * t) * decay(t, 16.0);
}

/* A wobbling swell: a bubble closing round somebody. */
static double bubble_wrap_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 420.0 + 700.0 * t + 70.0 * sin(TAU * 18.0 * t);
  return 0.4 * (sin(TAU * f * t) + 0.3 * sin(TAU * f * 2.0 * t)) * sin(3.14159 * fmin(1.0, t / 0.35));
}

/* A wet pop: a click of noise over a falling blip. */
static double bubble_pop_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.5 * (noise(r) - lp[0]);
  return (0.9 * lp[0] * decay(t, 45.0) + 0.5 * sin(TAU * (1100.0 - 3500.0 * t) * t) * decay(t, 22.0));
}

/* A gloopy heave: a low wobbling bloop with a wet hiss on top. */
static double slop_throw_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.25 * (noise(r) - lp[0]);
  double wobble = 1.0 + 0.4 * sin(TAU * 22.0 * t);
  return (0.5 * sin(TAU * (150.0 + 500.0 * t) * wobble * t) + 0.5 * lp[0]) * decay(t, 6.0);
}

/* A wet splat, then a glitchy bit-crushed fizz as the monster tears into noise. */
static double slop_dies_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.3 * (noise(r) - lp[0]);
  double splat = (0.9 * lp[0] + 0.6 * sin(TAU * (240.0 - 300.0 * t) * t)) * decay(t, 7.0);
  double crush = floor(noise(r) * 4.0) / 4.0 * 0.3 * (t > 0.15 ? decay(t - 0.15, 4.0) : 0.0);
  return splat + crush;
}

/* A snap and a rising, wobbling hum: the blade lights. */
static double saber_on_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)s;
  double hum = 0.35 * sin(TAU * (90.0 + 40.0 * t) * t) + 0.25 * sin(TAU * 2.0 * (90.0 + 40.0 * t) * t) +
               0.12 * sin(TAU * 7.0 * (90.0 + 40.0 * t) * t);
  double wobble = 1.0 + 0.2 * sin(TAU * 11.0 * t);
  double snap = t < 0.05 ? noise(r) * 0.6 * (1.0 - t / 0.05) : 0.0;
  double swell = fmin(1.0, t / 0.25) * decay(fmax(0.0, t - 0.4), 5.0);
  return snap + hum * wobble * swell;
}

/* A whoosh of noise rising and falling, and a zap sweeping down. */
static double force_burst_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.15 * (noise(r) - lp[0]);
  double whoosh = lp[0] * sin(3.14159 * fmin(1.0, t / 0.3)) * 1.2;
  double zap = 0.35 * sin(TAU * (1500.0 - 3200.0 * t) * t) * decay(t, 9.0);
  return whoosh * decay(t, 4.0) + zap;
}

/* A gorilla's growl: a deep sawtooth that rattles (a fast tremolo), with a rough hiss over it, dipping
 * in pitch as it goes, swelling in and trailing away. */
static double gorilla_growl_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s; /* [0] the roughness, [1] the low-passed voice */
  double pitch = 78.0 - 22.0 * t;
  double phase = pitch * t;
  double saw = 2.0 * (phase - floor(phase + 0.5));
  double rattle = 0.55 + 0.45 * sin(TAU * (24.0 + 6.0 * t) * t);
  lp[0] += 0.12 * (noise(r) - lp[0]);
  lp[1] += 0.18 * (saw * rattle - lp[1]);
  double envelope = fmin(1.0, t / 0.08) * (t < 0.55 ? 1.0 : decay(t - 0.55, 5.0));
  return (1.1 * lp[1] + 0.9 * lp[0] * rattle) * envelope;
}

/* A parachute opening: a puff of low-passed noise that swells and flaps, and a soft thump. */
static double parachute_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.08 * (noise(r) - lp[0]);
  double flap = 0.6 + 0.4 * sin(TAU * 28.0 * t * (1.0 - t));
  double puff = lp[0] * 3.0 * flap * fmin(1.0, t / 0.06) * decay(t, 6.0);
  double thump = 0.5 * sin(TAU * (90.0 - 60.0 * t) * t) * decay(t, 14.0);
  return puff + thump;
}

/* A terracotta vase on stone: a sharp crack, then a rattle of falling shards. */
static double vase_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)s;
  double crack = noise(r) * 0.9 * decay(t, 40.0);
  double body = 0.4 * sin(TAU * (520.0 - 600.0 * t) * t) * decay(t, 18.0);
  double shards = 0.0;
  for (int i = 1; i <= 5; i++) {
    double at = 0.06 + 0.055 * (double)i;
    if (t > at) shards += 0.28 * sin(TAU * (900.0 + 330.0 * (double)i) * (t - at)) * decay(t - at, 70.0);
  }
  return crack + body + shards;
}

/* Water rushing on: a band of hiss that swells fast and runs on, with a gurgling wobble. */
static double water_voice(double t, double d, rng_t* r, void* s) {
  double* lp = (double*)s;
  lp[0] += 0.35 * (noise(r) - lp[0]);
  lp[1] += 0.08 * (lp[0] - lp[1]);
  double band = lp[0] - lp[1];
  double gurgle = 0.75 + 0.25 * sin(TAU * (9.0 + 4.0 * sin(TAU * 1.3 * t)) * t);
  double envelope = fmin(1.0, t / 0.25) * (t < d - 0.4 ? 1.0 : (d - t) / 0.4);
  return band * 1.8 * gurgle * envelope;
}

/* A pipe: a falling square-wave slide with a wobble, ending in a soft thud. */
static double warp_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 620.0 - 520.0 * t + 40.0 * sin(TAU * 20.0 * t);
  double slide = 0.3 * square(f * t, 0.5) * (t < 0.5 ? 1.0 : decay(t - 0.5, 14.0));
  double thud = t > 0.45 ? 0.4 * sin(TAU * 90.0 * (t - 0.45)) * decay(t - 0.45, 18.0) : 0.0;
  return slide + thud;
}

/* A friend arrives: four bright notes climbing, C E G C. */
static double ally_arrive_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  static const double kNotes[4] = {523.25, 659.25, 783.99, 1046.5};
  int i = (int)(t / 0.09);
  if (i > 3) i = 3;
  double local = t - (double)i * 0.09;
  double f = kNotes[i];
  return 0.3 * (square(f * local, 0.5) * 0.6 + sin(TAU * f * local)) * (i < 3 ? decay(local, 6.0) : decay(local, 9.0));
}

/* Andrea's belly: a low thump, then a springy boing. */
static double ally_belly_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double thump = 0.7 * sin(TAU * (120.0 - 150.0 * t) * t) * decay(t, 16.0);
  double boing = t > 0.05 ? 0.35 * sin(TAU * (420.0 - 280.0 * (t - 0.05) + 60.0 * sin(TAU * 18.0 * t)) * (t - 0.05)) * decay(t - 0.05, 7.0) : 0.0;
  return thump + boing;
}

/* Alessandro: a rising whoosh of noise and a sharp crack. */
static double ally_kungfu_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += (0.1 + 0.5 * t) * (noise(r) - lp[0]);
  double whoosh = lp[0] * sin(3.14159 * fmin(1.0, t / 0.25)) * 1.4;
  double crack = t > 0.16 ? noise(r) * 0.8 * decay(t - 0.16, 40.0) : 0.0;
  double yell = 0.2 * sin(TAU * (500.0 + 500.0 * t) * t) * decay(t, 7.0);
  return whoosh * decay(t, 5.0) + crack + yell;
}

/* Giulia: a solid thud, noise over a falling low note. */
static double ally_punch_voice(double t, double d, rng_t* r, void* s) {
  (void)d;
  double* lp = (double*)s;
  lp[0] += 0.3 * (noise(r) - lp[0]);
  return (0.9 * lp[0] * decay(t, 28.0)) + 0.8 * sin(TAU * (160.0 - 400.0 * t) * t) * decay(t, 22.0);
}

static double spell_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double f = 500.0 + 1500.0 * t / 0.3 + 60.0 * sin(TAU * 30.0 * t);
  return 0.3 * (sin(TAU * f * t) + 0.5 * sin(TAU * f * 1.5 * t)) *
         decay(t, 5.0);
}

/* A slide whistle that swoops up with a wobble, then flops back down. */
static double skewer_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r; (void)s;
  double up = fmin(t, 0.26), down = fmax(t - 0.26, 0.0);
  double phase = 330.0 * up + 0.5 * 2600.0 * up * up + 1000.0 * down - 0.5 * 4200.0 * down * down;
  double wobble = 2.2 * sin(TAU * 24.0 * t);
  double pop = t < 0.015 ? 0.5 * noise(r) : 0.0;
  return (0.55 * sin(TAU * phase + wobble) + pop) * decay(t, 3.5);
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

/* Something coming up out of a block: three quick climbs on a thin pulse, each
 * starting higher and rising an octave with a wobble, then a twinkle on top. */
static double block_voice(double t, double d, rng_t* r, void* s) {
  (void)d; (void)r;
  double* phase = (double*)s;
  static const double starts[] = {392.0, 523.25, 659.25}; /* G4, C5, E5 */
  const double climb = 0.13;
  double f, amp;
  if (t < 3.0 * climb) {
    int k = (int)(t / climb);
    double u = (t - k * climb) / climb;
    f = starts[k] * pow(2.0, u) * (1.0 + 0.025 * sin(TAU * 28.0 * t));
    amp = 0.55 + 0.45 * decay(u, 3.0);
  } else {
    double since = t - 3.0 * climb;
    f = fmod(since, 0.08) < 0.04 ? 1567.98 : 2093.0; /* G6 and C7 in turn */
    amp = decay(since, 7.0);
  }
  phase[0] += f / MIXER_RATE; /* glides stay smooth when the pitch moves */
  return 0.3 * amp * square(phase[0], 0.25);
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

/* Brings a clip's loudest sample up to `peak`: speech comes out quiet. */
static void normalise(sample_t* clip, double peak) {
  int16_t* data = (int16_t*)clip->data;
  int loudest = 1;
  for (int i = 0; i < clip->length; i++)
    if (abs(data[i]) > loudest) loudest = abs(data[i]);
  double gain = peak / loudest;
  for (int i = 0; i < clip->length; i++) data[i] = (int16_t)(data[i] * gain);
}

/* A clip of our own, so sfx_free can release every clip alike. */
static bool copy_clip(sample_t* clip, const int16_t* data, int length) {
  int16_t* copy = malloc((size_t)length * sizeof(int16_t));
  if (!copy) return false;
  memcpy(copy, data, (size_t)length * sizeof(int16_t));
  *clip = (sample_t){copy, length};
  return true;
}

bool sfx_build(sample_t clips[SFX_COUNT]) {
  for (int i = 0; i < SFX_COUNT; i++) clips[i] = (sample_t){NULL, 0};
  bool ok = true;
  ok = ok && render(&clips[SFX_JUMP], 0.22, jump_voice, 1);
  ok = ok && render(&clips[SFX_SHOOT], 0.22, shoot_voice, 2);
  ok = ok && render(&clips[SFX_SPELL], 0.45, spell_voice, 3);
  ok = ok && render(&clips[SFX_SLOP_THROW], 0.5, slop_throw_voice, 27);
  ok = ok && render(&clips[SFX_SLOP_DIES], 0.9, slop_dies_voice, 28);
  ok = ok && render(&clips[SFX_SABER_ON], 1.0, saber_on_voice, 29);
  ok = ok && render(&clips[SFX_FORCE_BURST], 0.4, force_burst_voice, 30);
  ok = ok && render(&clips[SFX_GORILLA_GROWL], 0.9, gorilla_growl_voice, 31);
  ok = ok && copy_clip(&clips[SFX_VOICE_SLOP], kVoiceSlop, kVoiceSlopLength);
  ok = ok && render(&clips[SFX_WARP], 0.65, warp_voice, 32);
  ok = ok && render(&clips[SFX_PARACHUTE], 0.5, parachute_voice, 37);
  ok = ok && render(&clips[SFX_VASE], 0.5, vase_voice, 38);
  ok = ok && render(&clips[SFX_WATER], 1.6, water_voice, 39);
  ok = ok && render(&clips[SFX_ALLY_ARRIVE], 0.55, ally_arrive_voice, 33);
  ok = ok && render(&clips[SFX_ALLY_BELLY], 0.5, ally_belly_voice, 34);
  ok = ok && render(&clips[SFX_ALLY_KUNGFU], 0.4, ally_kungfu_voice, 35);
  ok = ok && render(&clips[SFX_ALLY_PUNCH], 0.2, ally_punch_voice, 36);
  ok = ok && render(&clips[SFX_BUBBLE_SHOT], 0.16, bubble_shot_voice, 21);
  ok = ok && render(&clips[SFX_BUBBLE_WRAP], 0.5, bubble_wrap_voice, 22);
  ok = ok && render(&clips[SFX_BUBBLE_POP], 0.3, bubble_pop_voice, 23);
  ok = ok && render(&clips[SFX_SKEWER], 0.42, skewer_voice, 20);
  ok = ok && render(&clips[SFX_MELEE], 0.18, melee_voice, 4);
  ok = ok && render(&clips[SFX_ZOMBIE_BOOM], 0.6, zombie_boom_voice, 5);
  ok = ok && render(&clips[SFX_BUG_BOOM], 0.3, bug_boom_voice, 6);
  ok = ok && render(&clips[SFX_COIN], 0.35, coin_voice, 7);
  ok = ok && render(&clips[SFX_PICKUP], 0.4, pickup_voice, 8);
  ok = ok && render(&clips[SFX_DRINK], 1.0, drink_voice, 9);
  ok = ok && render(&clips[SFX_HURT], 0.4, hurt_voice, 10);
  ok = ok && render(&clips[SFX_BUMP], 0.15, bump_voice, 11);
  ok = ok && render(&clips[SFX_STOMP], 0.2, stomp_voice, 12);
  ok = ok && render(&clips[SFX_BLOCK], 0.68, block_voice, 13);
  ok = ok && render(&clips[SFX_DEATH], 1.2, death_voice, 15);
  ok = ok && render(&clips[SFX_WIN], 1.5, win_voice, 16);
  ok = ok && render(&clips[SFX_HEAL], 0.35, heal_voice, 17);
  ok = ok && render(&clips[SFX_VOICE_FABIO], 1.5, speech_voice, 18);
  ok = ok && copy_clip(&clips[SFX_VOICE_CURSE], kVoiceCurse, kVoiceCurseLength);
  for (int i = 0; i < DRUM_WORDS; i++)
    ok = ok && copy_clip(&clips[SFX_DRUM_BOOM + i], kDrumClips[i].data, kDrumClips[i].length);
  ok = ok && render(&clips[SFX_BOOK_READER_BOOM], 1.1, book_reader_boom_voice, 19);
  if (ok) {
    normalise(&clips[SFX_VOICE_FABIO], 31000.0);
    normalise(&clips[SFX_GORILLA_GROWL], 30000.0);
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
    case EVENT_BLOCK_OPENS: *volume = 0.6f; return SFX_BLOCK;
    case EVENT_DEATH: return SFX_DEATH;
    case EVENT_WIN: return SFX_WIN;
    case EVENT_HEAL: return SFX_HEAL;
    case EVENT_BOOK_READER_DIES: *volume = 0.9f; return SFX_BOOK_READER_BOOM;
    case EVENT_MONK_DIES: *volume = 1.0f; return SFX_BOOK_READER_BOOM;
    case EVENT_CHEF_DIES: *volume = 1.0f; return SFX_BOOK_READER_BOOM;
    case EVENT_GORILLA_DIES: *volume = 1.0f; return SFX_ZOMBIE_BOOM;
    case EVENT_BANANA_THROWN: *volume = 0.5f; return SFX_MELEE;
    case EVENT_DEMON_DIES: *volume = 0.9f; return SFX_BOOK_READER_BOOM;
    case EVENT_DEMON_CASTS: *volume = 0.6f; return SFX_SPELL;
    case EVENT_DRUMS_START: *volume = 0.8f; return SFX_PICKUP;
    case EVENT_DRUM_BOOM: *volume = 0.7f; return SFX_DRUM_BOOM;
    case EVENT_DRUM_BADABOOM: *volume = 0.7f; return SFX_DRUM_BADABOOM;
    case EVENT_DRUM_TASH: *volume = 0.6f; return SFX_DRUM_TASH;
    case EVENT_DRUM_BODOBOOOM: *volume = 0.7f; return SFX_DRUM_BODOBOOOM;
    case EVENT_DRUM_BOING: *volume = 0.6f; return SFX_DRUM_BOING;
    case EVENT_DRUM_DISH: *volume = 0.6f; return SFX_DRUM_DISH;
    case EVENT_BUBBLE_SHOT: *volume = 0.35f; return SFX_BUBBLE_SHOT;
    case EVENT_BUBBLE_WRAPS: *volume = 0.7f; return SFX_BUBBLE_WRAP;
    case EVENT_BUBBLE_POPS: *volume = 0.9f; return SFX_BUBBLE_POP;
    case EVENT_SLOP_SPEAKS: *volume = 0.9f; return SFX_VOICE_SLOP;
    case EVENT_GORILLA_GROWLS: *volume = 1.0f; return SFX_GORILLA_GROWL;
    case EVENT_SLOP_THROWN: *volume = 0.7f; return SFX_SLOP_THROW;
    case EVENT_SLOP_DIES: *volume = 1.0f; return SFX_SLOP_DIES;
    case EVENT_LIGHTSABER: *volume = 0.9f; return SFX_SABER_ON;
    case EVENT_FORCE_BURST: *volume = 0.7f; return SFX_FORCE_BURST;
    case EVENT_WARP: *volume = 0.8f; return SFX_WARP;
    case EVENT_PARACHUTE_OPENS: *volume = 0.8f; return SFX_PARACHUTE;
    case EVENT_VASE_SMASHES: *volume = 0.8f; return SFX_VASE;
    case EVENT_WATER_ON: *volume = 0.7f; return SFX_WATER;
    case EVENT_ALLY_ARRIVES: *volume = 0.9f; return SFX_ALLY_ARRIVE;
    case EVENT_ALLY_BELLY: *volume = 1.0f; return SFX_ALLY_BELLY;
    case EVENT_ALLY_KUNGFU: *volume = 0.9f; return SFX_ALLY_KUNGFU;
    case EVENT_ALLY_PUNCH: *volume = 0.9f; return SFX_ALLY_PUNCH;
    case EVENT_SKEWER_THROWN: *volume = 0.8f; return SFX_SKEWER;
    case EVENT_HELICOPTER_BOARDS: *volume = 0.8f; return SFX_PICKUP;
    case EVENT_HELICOPTER_EXPLODES: *volume = 1.0f; return SFX_BOOK_READER_BOOM;
    case EVENT_KIND_COUNT: break;
  }
  return SFX_BUMP;
}

bool event_makes_fabio_swear(event_kind_t kind) { return kind == EVENT_HURT; }

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
