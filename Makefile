.DEFAULT_GOAL := all

CC ?= cc
SDL2_CONFIG ?= $(shell command -v sdl2-config 2>/dev/null || echo /opt/homebrew/bin/sdl2-config)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
    INSTALL_CMD = git submodule update --init --recursive && sudo apt-get update && sudo apt-get install -y libsdl2-dev libsdl2-image-dev python3-pil
    DEV_INSTALL_CMD = sudo apt-get update && sudo apt-get install -y cpplint clang-format
else ifeq ($(UNAME_S), Darwin)
    INSTALL_CMD = git submodule update --init --recursive && brew install sdl2 sdl2_image && python3 -m pip install pillow
    DEV_INSTALL_CMD = brew install cpplint clang-format
else
    $(error Unsupported platform)
endif

# The engine is built by its own makefile and consumed through what it exports:
# a library, the directories holding its public headers, and the flags to link
# it. Nothing here names its sources, its internals or what it runs on.
ENGINE_DIR = engine
ENGINE_LIB = $(ENGINE_DIR)/libsdl2d.a
# TrueType text is optional in the engine, and this game does not use it.
ENGINE_MAKE = $(MAKE) --no-print-directory -C $(ENGINE_DIR) WITH_TTF=0
ENGINE_INCLUDES := $(foreach dir,$(shell $(ENGINE_MAKE) -s print-public-includes),-I$(ENGINE_DIR)/$(dir))
ENGINE_LFLAGS := $(shell $(ENGINE_MAKE) -s print-link-flags)

GAME_DIRS = $(wildcard game/src/*)
GAME_SRC = $(foreach dir,$(GAME_DIRS),$(wildcard $(dir)/*.c))
OWN_HEADERS = $(foreach dir,$(GAME_DIRS),$(wildcard $(dir)/*.h))
TEST_SRC = $(wildcard tests/*.c)

GAME_OBJ = $(GAME_SRC:.c=.o)
GAME_DEP = $(GAME_OBJ:.o=.d)
-include $(GAME_DEP)

OPT ?= -O2
# The game is never compiled against the platform library: its compile line
# carries no platform include path, and stub headers searched first turn any
# attempt to include the library into an error.
GAME_CFLAGS := -ggdb3 $(OPT) --std=c99 -Wall -Wextra -pedantic-errors -D_DEFAULT_SOURCE \
               -Iscripts/boundary_stubs $(ENGINE_INCLUDES) $(foreach dir,$(GAME_DIRS),-I$(dir))

TARGET = super-fabio-bros
SPRITES = assets_cooked/sprites/manifest.txt

.PHONY: all assets install dev_install clean lint format test sanitize run snapshot boundary FORCE

all: $(TARGET) assets

$(TARGET): $(GAME_OBJ) $(ENGINE_LIB)
	$(CC) -o $@ $(GAME_OBJ) $(ENGINE_LIB) $(ENGINE_LFLAGS)

# Always asked, and the engine's makefile decides whether anything is stale.
$(ENGINE_LIB): FORCE
	$(ENGINE_MAKE) libsdl2d.a

FORCE:

# Cuts the generated sprite sheets in assets/source into transparent PNGs.
$(SPRITES): scripts/cook_sprites.py $(wildcard assets/source/*.jpeg assets/source/*.png)
	python3 scripts/cook_sprites.py

assets: $(SPRITES)

$(GAME_OBJ): %.o: %.c
	$(CC) $(GAME_CFLAGS) -MMD -MP -c -o $@ $<

# The game must stay on the engine's side of the line. Independent of any
# earlier build: the source check, every game file compiled afresh with the
# compiler's include environment cleared, and the engine's public headers
# checked for the platform library.
boundary:
	python3 scripts/check_boundary.py
	@for source in $(GAME_SRC); do \
	  env -u CPATH -u C_INCLUDE_PATH -u CPLUS_INCLUDE_PATH $(CC) $(GAME_CFLAGS) -fsyntax-only $$source || exit 1; \
	done
	python3 $(ENGINE_DIR)/tests/run_tests.py --check-headers

install:
	$(INSTALL_CMD)

dev_install:
	$(DEV_INSTALL_CMD)

# The engine is a submodule and lints and formats itself; only the game's own
# files are checked here.

lint:
	cpplint --quiet --root=. $(GAME_SRC) $(OWN_HEADERS) $(TEST_SRC)

format:
	clang-format -i $(GAME_SRC) $(OWN_HEADERS) $(TEST_SRC)

test: assets boundary
	python3 tests/run_tests.py

sanitize: assets boundary
	python3 tests/run_tests.py --sanitize

run: all
	./$(TARGET)

# A frame of a bot playing for 20 seconds, and the title screen.
snapshot: all
	./$(TARGET) --seed 7 --snapshot snapshot.bmp --seconds 20
	./$(TARGET) --snapshot snapshot_title.bmp --title

clean:
	$(ENGINE_MAKE) clean
	rm -f $(GAME_OBJ) $(GAME_DEP) $(TARGET) snapshot*.bmp
