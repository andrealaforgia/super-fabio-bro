# ADR-002: use the shared SDL 2D engine

The game used to carry its own small engine (`engine/`: RNG, stepper, controls,
mixer, SDL drawing surface, 5x7 font). Asteroids and other games use the
shared `sdl-2d-game-engine`, so the same ideas were being written twice.

**Decision.** The game uses the shared engine as a git submodule in `engine/`,
tracking the branch `shared-engine-for-platformers` until it is merged to the
engine's trunk, and carries no engine code of its own. What the game needed
that the engine lacked was added to the engine, not kept here:

- a seedable `rng_t`, and random helpers that take it instead of `rand()`;
- `stepper_plan` and a high-resolution clock;
- `controls_t` as held and pressed bits of a mask the game defines, with key
  bindings, replacing the timer-debounced keyboard functions;
- a window configured by `graphics_config_t`, a view of fixed height whose
  width follows the window, one `render_sprite` call (float rectangle, flip,
  alpha, rotation, tint), textures from pixels, a built-in 5x7 pixel font and
  frame capture to BMP;
- a software mixer with a music callback, replacing SDL_mixer, so generated
  samples and WAV files share one path;
- command-line options a game can extend, and pool walking and release by
  object.

**Shared structures.** The world keeps enemies, pickups, bullets, effects,
particles, rays and block bumps in engine object pools, announces what happens
through the engine's event system (the audio subscribes a sound to each event
kind), and reads its options with the engine's parser.

**Consequences.**
- The engine's own tests cover the engine; the game's headless tests compile
  only the engine's pure parts (RNG, stepper, controls, mixer, events, pool).
- Rendering is checked by comparing snapshots with a fixed seed against the
  ones from the commit before the migration (tag
  `checkpoint-before-engine-migration`): they were identical after every step.
- Audio and the live window are not covered by snapshots; they were run with
  SDL's dummy drivers and need an ear and an eye.
- SDL2_mixer is no longer needed (OGG files are no longer read); SDL2_ttf is
  optional in the engine (`make WITH_TTF=0`), and this game does not use it.
- Options now use `--name=value` (for example `--seed=7`).
