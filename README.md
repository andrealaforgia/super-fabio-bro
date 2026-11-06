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
  They turn round every couple of seconds (frames in
  `assets/source/question_block.png`). They come in four colours (yellow,
  blue, grey, orange), each block keeping its own.
- The pistol fires bullets that explode zombies and bugs. Each pistol adds
  15 bullets, counted next to it at the top of the screen; when they run out
  the pistol is gone until you find another.
- A potion gives 14 seconds of the TDD spell: shooting casts a piercing ray
  through everything in line, gun or no gun. A bar shows it fading.
- When the hooded monk goes up in smoke, a demon rises from it where he stood,
  fading in as the smoke fades out (he does no harm until he is all there).
  He stays there, turned towards Fabio, and every few seconds casts a fireball
  straight at him while he is on screen; it costs a heart, flies past everybody
  else and goes out against a wall. Walking into him hurts; a stomp, a bullet,
  the TDD ray or the pickaxe makes him clutch himself and dissolve into smoke
  (500 points). His frames are cut from `assets/source/demon.jpeg`.
- Everyone who turns up on the ground has ground under the whole of him, never
  half over a pit or a gap.
- Characters arrive one at a time, about 2.3 seconds apart at the start and
  1.4 near the castle, and none at all while six (nine near the castle) are
  already within a screen of Fabio. The book readers, monks, chefs and gorillas
  come at least 6 seconds apart, never more than two about at once. All of
  this follows one setting, `SPAWN_PACE` in `game/src/sim/world.h`: 2 is twice
  the calm base rate. Each kind waits its turn: the one that has waited longest goes next.
- Zombies walk in from both edges, more often as you go.
  Bugs crawl or fly. Touching them costs a heart; mushrooms give one back;
  pits cost one too.
- Every 10 to 18 seconds a Book Reader turns up ahead of you, up to two at
  a time, reading Fabio's book. He never moves from where he appears and is
  harmless: touching him, jumping on him or swinging the pickaxe does
  nothing. He reads for a few seconds, laughs his head off for a couple,
  reads again, laughs again, and so on. A bullet or the TDD ray sets him off:
  he swells, flashes and bursts in a shower of pages (500 points). His frames
  are in
  `assets/book_reader_spritesheet.png`.
- Every run has its own level layout and its own look: the seed picks the
  time of day (day, sunset, dusk, dawn or overcast) and where the mountains
  and clouds start. The first few screens are always the same by design.
- Every 13 to 20 seconds a monk or a skeleton turns up ahead of you, up to two
  at a time. The monk keeps saying "I WON'T PAIR!", the skeleton "I LOVE
  PRS!". The monk never moves: he prays while Fabio is far and holds up his cross
  as Fabio gets close. The skeleton walks slowly towards Fabio, hopping
  blocks and falling into pits like a zombie. Walking into either hurts Fabio
  and throws him back, like a zombie, and jumping on one squashes it. A bullet, the TDD ray or the pickaxe
  also sets them off: the monk goes up in an explosion, the skeleton fades into
  dust in half a second (300 points). The monk's standing frames are cut
  from `assets/source/monk_cross.jpeg`, his blast and the skeleton from
  `assets/source/monk.png` and `assets/source/retro_characters.png`. They
  always turn to face Fabio, and speak in the zombies' bubble style (made by
  `scripts/make_bubbles.py`, kept in `assets/source/bubbles.png`).
- Now and then (the first after about 12 seconds, then every 20 to 35) a chef
  turns up ahead of you on a flat stretch of grass, asking "ELEPHANT
  CARPACCIO, ANYONE?". While Fabio is far he smiles; once Fabio is within
  about 150 pixels he swings his knife, and the blade hurts Fabio from beyond
  the chef's own body. Jumping on the chef (or a bullet, the TDD ray or the
  pickaxe) kills him: he bursts into smoke and a shower of fifty
  vegetables of every kind, flung every way, swelling as they fade away; they
  hurt nobody (600 points). The vegetables are cut from
  `assets/source/vegetables.jpeg`. Book
  readers, monks and chefs never appear on top of one another. Sprites are cut
  from `assets/source/chef_happy.jpeg` and `assets/source/chef_angry.jpeg`.
- The level has moving planks. Over the wider pits a wooden plank swings from
  one side to the other, level with the ground, and beside some of the high
  gold ledges a steel lift rises and falls on its cable. Fabio can jump up
  through a plank from below, lands on it from above and is carried along
  with it. Planks do nothing for the enemies.
- Nothing is lost in a pit for good: a zombie, skeleton, beetle, book reader,
  monk or chef that ends up in one leaps out again, towards Fabio's side if
  it can, and up to five tiles of wall are no obstacle. Now and then one is
  sent up out of a pit ahead of Fabio.
- Every 15 to 23 seconds a gorilla turns up ahead of you, one at a time. He
  stands where he appears, always turned towards Fabio, and goes round and
  round: he stands, roars (a deep, rattling growl sounds as the roar begins), beats his chest
  and throws a banana. The banana
  spins along an arc aimed roughly at Fabio (anywhere up to 90 pixels either
  side of him, so he can often dodge it) and costs him a heart, like any hit, but
  kills any zombie, bug or monk it hits first on the way; only the gorilla who
  threw it is safe, and walls squash it. He throws whenever he
  is on screen, from over his head so the banana clears a low wall in front of
  him. Walking into him hurts like a zombie; a stomp, a
  bullet, the TDD ray or the pickaxe knocks him out (400 points), and he
  bursts into a shower of fifty bananas that fly every way and fade out; they
  are innocent and hurt nobody. His frames
  are cut from `assets/source/gorilla.jpeg`.
- There is one helicopter in every level, resting on the first long stretch of
  open grass about a quarter of the way in. Land on it from above (a jump will
  do) and Fabio climbs in; walking into it from the side does nothing. Flying,
  space or up climbs, down sinks and with neither he hangs in the air; he
  cannot leave the screen or sink into a pit, and nothing can hurt him up
  there. The shoot key throws lamb skewers instead of bullets, with no limit:
  each leaves at 45 degrees and arcs, turning flat at the top of its flight
  and then another 80 degrees, stopping 10 short of pointing at the ground, and kills what it hits like a bullet (sprite: `assets/source/skewer.jpeg`).
  The fuel lasts 30 seconds (a small bar over the helicopter, blinking in the
  last five). Then the helicopter blows up and is gone for good, and Fabio
  falls, unharmed (a pit below still costs a heart, as ever). The resting
  helicopter is cut from `assets/source/helicopter.jpeg`; flying and
  exploding, from `assets/source/fabio_helicopter.jpeg`.
- Fabio's drum kit first stands on the open grass about a quarter of the way into the level (past column 90, never near the start), and then another one every 70 to 100 columns or so all along the level (never on the helicopter's grass). Jump
  into it (walking past does nothing) and he sits down and plays by himself
  for 10 seconds, with no keys needed and nothing able to hurt him, his eight
  frames flashing by every 0.05 seconds. All the while the kit shouts BOOM,
  BADABOOM, TASH, BODOBOOOM, BOING and DISH, each with its own drum sound: a
  word swells into a ring round the kit, out to 320 pixels, then fades, and
  kills any enemy it touches. Tap left, right or jump to get up early (left and right step out that way, jump hops out). Otherwise he hops out when the time is up, and every new jump into the kit starts a fresh 10 seconds. The
  kit at rest is from `assets/source/drum_kit.png`, the playing frames from
  the third row of `assets/source/drummer_frantic.png`, and the sounds are CC0
  drum samples from the Sonic Pi library (`assets/source/sounds`), cut and
  mixed by `scripts/make_drum_sounds.py` into `game/src/audio/drum_sounds.c`.
- The soap bubble shotgun comes out of a `?` block (one in the opening row, then
  now and then along the level) and gives 50 shots, counted at the top of the
  screen. It takes the pistol's place while it has shots (the pistol keeps its
  bullets for after). Hold the shoot key and it lets go two bubbles every 0.12
  seconds in a loose spread. Each bubble keeps swelling as it flies, from a speck
  to nearly 100 pixels across, and bursts against a wall or after 1.8 seconds. The
  first enemy a bubble touches is wrapped in it and floats up, helpless and
  harmless to touch, and 1.5 seconds later bubble and enemy burst together (the
  usual points). The helicopter's skewers and the TDD spell still come first.
  Fabio's frames are cut from `assets/source/fabio_bubble_gun.png`, the gun and
  the round bubble from `assets/source/bubble_gun.png`.
- The AI slop monster (every 28 to 45 seconds, one at a time, never on top of
  another special) stands where he appears, turned towards Fabio, a hulking
  purple and teal heap of melted generated nonsense with too many fingers and teeth,
  wearing its AI on its sleeve: glowing sparkle stars, a chatbot bubble with typing dots, a network
  of nodes and wires, a circuit chip and a `>_` prompt.
  He goes round and round: he says "Certainly! Here is some slop." in a recorded robot voice
  (`scripts/make_voice_slop.py`), drips, winds up a ball of slop over his head and hurls
  it at Fabio on an arc, aimed anywhere up to 90 pixels either side of him. The ball
  costs a heart, splats against walls and the ground, and hurts nobody else, unlike
  the gorilla's banana. Walking into him hurts; a stomp, a bullet, the TDD ray, the
  pickaxe or a bubble kills him (450 points), and he tears into glitchy noise and
  bursts into a shower of thirty blobs of slop with eyes and fingers. His frames are cut
  from `assets/source/slop_monster.png`, generated with Codex to match the other sheets.
- The green lightsaber comes out of a `?` block (one in the opening row, then now and
  then along the level). Picking it up turns Fabio into a Jedi knight for 20 seconds (a
  green bar, THE FORCE, shows it running out): robes, a trimmed beard and the saber in
  hand. The shoot key swings it and sends a green burst flying ahead, one every 0.4
  seconds, that destroys every enemy it passes through (about 570 pixels, stopped
  by a wall). The pistol keeps its bullets meanwhile. The moment he picks it up an old
  knight's pixel portrait fades in at the top left under the scoreboard, his mouth moving
  as the line "MAY THE FORCE BE WITH YOU FABIO!" shows beside him, and fades away
  after 4.5 seconds. Frames are cut from `assets/source/fabio_jedi.png` and
  `jedi_items.png`, generated with Codex to match the other sheets.
- The level changes scenery as it goes: forest for the first 150 columns, then desert, then
  snow up to the castle. Each zone has its own ground (sand over sandstone, snow over
  frozen earth), its own decorations (cacti, a dead tree, a bleached skull and boulders; snowy
  pines, a snowman, ice crystals), its own sky and weather (amber with sand blowing
  across, pale with snow falling), blending into one another as the screen crosses the
  border. The snow zone is icy: Fabio is slow to get going and slow to stop there. The
  ground tiles and decorations are cut from `assets/source/biome_ground.png` and
  `biome_decor.png`, generated with Codex to match the other sheets.
- Every pipe is a warp pipe, marked by a green arrow and a glow. Stand on the cap and tap
  down: Fabio sinks into the pipe, the screen fades, and he rises out of a secret cave
  under it. Every cave is a closed treasure room (always the same for the same pipe
  and seed): three prize blocks (a lightsaber or the bubble gun, a potion, and a pistol or
  a mushroom), trails of coins and a high gold ledge reached by a stone stair, crystals,
  glowing mushrooms and stalactites, and a guardian standing over it all (a gorilla
  or the AI slop monster). A pipe at each end leads back up, out of the pipe he went down;
  that pipe stays shut afterwards. Nothing spawns in a cave and nothing in it can
  end the run. The cave's tiles, crystals and stalactites are also in the two sheets above.
- Fabio has three friends who turn up when he picks up the right thing (all three are in the
  opening row of `?` blocks, and now and then along the level). A sushi dumpling brings
  Andrea, short-black-haired, black-bearded and round-bellied; a Chinese dragon brings Alessandro, a
  chubby, scruffy, bearded kung fu grandmaster in black glasses; a pair of boxing gloves brings
  Giulia, a dark-haired boxer. Each arrives in a poof, stays 30 seconds (a chip at the
  top right shows who is here and how long he has left; another pickup of the same
  kind gives him another thirty seconds, never a twin) and fights alone, nothing able to hurt
  him. They stand a few steps ahead of Fabio, each at his own distance (Andrea 90 pixels,
  Alessandro 150, Giulia 210), and go round to the other side when Fabio turns. Andrea
  strokes his bare belly now and then, and the wobble (a big golden burst with a glow, stars and
  sparks) kills everything about 95 pixels round him. Alessandro runs at any enemy within 260 pixels and strikes it with kung fu; Giulia
  does the same within 230 and punches. They hop over low walls and leap the holes in the ground (even the widest), turn up at Fabio's side if
  left far behind or fallen into a pit, and go down warp pipes with him. All three are
  cut from `assets/source/ally_andrea.png`, `ally_alessandro.png`, `ally_giulia.png` and
  `ally_items.png`, generated with Codex to match the other sheets.
- Whenever points are scored (a kill, a coin, a pickup) the amount appears
  where it was earned, floats upwards and fades away.

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
