# Super Fabio Bros

A parody of Super Mario Bros, dedicated to my friend Fabio. Fabio runs and
jumps through a scrolling world of mountains, pines and floating bricks while
**I DON'T TEST!** zombies and giant bugs close in on him. Find the pistol,
find the TDD potion, drink it and cast a spell until it fades. Reach the
castle.

C99 and SDL2. The engine is the shared
[sdl-2d-game-engine](https://github.com/andrealaforgia/sdl-2d-game-engine),
a git submodule in `engine/`, with the game on top: rules that run without a
window so they can be tested, and an offline step that cooks the art.

## Setup

```
git clone --recurse-submodules git@github.com:andrealaforgia/super-fabio-bro.git
make install        # the engine submodule, SDL2 and SDL2_image, and Pillow
make                # builds the game and cooks the sprites
make run
```

Already cloned without `--recurse-submodules`? `make install` fetches the
engine too (or `git submodule update --init`).

The window opens as large as the desktop allows and resizes freely; F11 is
full screen. The world is always one screen tall and as wide as the window.

## Playing

The cover shows first, with Fabio saying "It's me! Fabio!". Press space to
start.

| Key | Does |
|---|---|
| arrows, or A and D | move |
| space, Z, up | jump (hold for height; land on enemies to squash them) |
| shift | run |
| enter, control | shoot the pistol; under the spell, cast the ray |
| K, X | swing the pickaxe |
| E, Q | drink a TDD potion |
| P, escape | pause |
| M | music on or off |
| S | sound effects and voices on or off |
| F11 | full screen |

- Hit `?` blocks from below. The first row holds the pistol and a potion.
- The pistol fires bullets that explode zombies and bugs. Each pistol adds
  15 bullets, counted next to it at the top of the screen; when they run out
  the pistol is gone until you find another.
- A potion gives 14 seconds of the TDD spell: shooting casts a piercing ray
  through everything in line, gun or no gun. A bar shows it fading.
- Zombies walk in from both edges at regular intervals, faster as you go.
  Bugs crawl or fly. Touching them costs a heart; mushrooms give one back;
  pits cost one too.

## Build, test, tools

```
make test           # every test group, no window needed
make sanitize       # the same under AddressSanitizer and UBSan
make snapshot       # a bot plays 20 s, saved to snapshot.bmp, plus the title
./super-fabio-bros --showcase --snapshot=show.bmp   # a staged fight
./super-fabio-bros --delay=10   # a countdown first, to start a screen recording
./super-fabio-bros --help       # every option, the engine's and the game's
```

## Layout

| Path | Holds |
|---|---|
| `engine/` | the shared engine (submodule): window and view, sprites, pixel font, software mixer, controls, RNG, stepper, object pool, event system, command line |
| `game/src/level` | tile map and the seeded level generator |
| `game/src/sim` | the whole game: player, enemies, spawner, combat, pickups, bot |
| `game/src/render` | poses, procedural scenery, the renderer |
| `game/src/audio` | synthesised effects, the "It's me! Fabio!" voice, the music |
| `game/src/assets` | the sprite manifest |
| `game/src/main` | the loop, the key bindings, the options |
| `scripts/cook_sprites.py` | cuts `assets/source/*.jpeg` into transparent PNGs |

All sound is synthesised at start-up, voice included (formant synthesis). The
mountains, trees, clouds and castle are drawn procedurally. Fabio, the
zombies, the bugs, the blocks and the pipes come from the generated sheets in
`assets/source`.

The game depends on the engine and never the other way round. Files named
`sdl_*.c` are the game's only ones that touch SDL, and the tests leave them
out, along with the engine's SDL parts, which have their own tests in the
engine repository. An autoplaying bot proves every generated level can be
finished.
