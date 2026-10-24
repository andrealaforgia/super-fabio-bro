# ADR-001: language, structure and testing

C99 and SDL2, following gta-like: an `engine/` that never includes `game/`,
a `game/src` split by concern, and a Makefile that builds everything.

- **2D with SDL_Renderer**, not OpenGL: the game is sprites and tiles.
- **Rules apart from drawing.** `game/src/sim` has no SDL; `world_update`
  takes explicit time and a controls struct, so tests play the game.
- **Seeded level generator** with a bot test that walks every level.
- **Art cooked offline.** The generated JPEG sheets are cut by measured
  rectangles into PNGs plus a manifest; `make assets` does it.
- **No audio files.** Effects, the spoken title and the music are synthesised.
- **World units:** 32 px tiles, the view is 544 units tall, scaled to the window.
