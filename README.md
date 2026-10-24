# Super Fabio Bros

A parody of Super Mario Bros, dedicated to my friend Fabio. Fabio runs and
jumps through a scrolling world of mountains, pines and floating bricks while
**I DON'T TEST!** zombies and giant bugs close in on him. Find the pistol,
find the TDD potion, drink it and cast a spell until it fades. Reach the
castle.

C99 and SDL2, built the same way as `gta-like`: a small engine, a game layer
on top, rules that run without a window so they can be tested, and an offline
step that cooks the art.

## Setup

```
make install        # SDL2 and SDL2_image via Homebrew (or apt), and Pillow
make                # builds the game and cooks the sprites
make run
```

The window opens as large as the desktop allows and resizes freely; F11 is
full screen. The world is always one screen tall and as wide as the window.

## Playing

The cover shows first, with Fabio saying "It's me! Fabio!". Press space to
start.

| Key | Does |
|---|---|
| arrows or WASD | move |
| space, Z, up | jump (hold for height; land on enemies to squash them) |
| shift | run |
| J, F, control | shoot the pistol; under the spell, cast the ray |
| K, X | swing the pickaxe |
| E, Q | drink a TDD potion |
| P, escape | pause |
| M | music on or off |
| F11 | full screen |

- Hit `?` blocks from below. The first row holds the pistol and a potion.
- The pistol fires bullets that explode zombies and bugs.
- A potion gives 14 seconds of the TDD spell: shooting casts a piercing ray
  through everything in line, gun or no gun. A bar shows it fading.
- Zombies walk in from both edges at regular intervals, faster as you go.
  Bugs crawl or fly. Touching them costs a heart; mushrooms give one back;
  pits cost one too.

## The demo

```
./super-fabio-bros --demo
```

Fabio plays himself, through the real game: the cover and "It's me! Fabio!",
a walk, a zombie stomped, another that pushes him back before he stomps it,
the same with bugs, a `?` block that gives the TDD potion, the spell blasting
zombies and bugs, a second block with the pistol, and shooting. Escape quits.

## Build, test, tools

```
make test           # every test group, no window needed
make sanitize       # the same under AddressSanitizer and UBSan
make snapshot       # a bot plays 20 s, saved to snapshot.bmp, plus the title
./super-fabio-bros --showcase --snapshot show.bmp   # a staged fight
./super-fabio-bros --help
```

## Layout

| Path | Holds |
|---|---|
| `engine/core` | deterministic RNG, frame stepper |
| `engine/math` | vectors and rectangles |
| `engine/platform` | controls with edge detection, SDL keyboard |
| `engine/renderer` | SDL 2D drawing surface and the 5x7 arcade font |
| `engine/audio` | pure voice mixer, SDL device |
| `game/src/level` | tile map and the seeded level generator |
| `game/src/sim` | the whole game: player, enemies, spawner, combat, pickups, bot |
| `game/src/render` | poses, procedural scenery, the renderer |
| `game/src/audio` | synthesised effects, the "It's me! Fabio!" voice, the music |
| `game/src/assets` | the sprite manifest |
| `scripts/cook_sprites.py` | cuts `assets/source/*.jpeg` into transparent PNGs |

All sound is synthesised at start-up, voice included (formant synthesis). The
mountains, trees, clouds and castle are drawn procedurally. Fabio, the
zombies, the bugs, the blocks and the pipes come from the generated sheets in
`assets/source`.

Nothing in `engine/` includes `game/`. Files named `sdl_*.c` are the only ones
that touch SDL, and the tests leave them out. An autoplaying bot proves every
generated level can be finished.
