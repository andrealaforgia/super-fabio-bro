.DEFAULT_GOAL := all

CC ?= cc
SDL2_CONFIG ?= $(shell command -v sdl2-config 2>/dev/null || echo /opt/homebrew/bin/sdl2-config)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S), Linux)
    INSTALL_CMD = sudo apt-get update && sudo apt-get install -y libsdl2-dev libsdl2-image-dev python3-pil
    DEV_INSTALL_CMD = sudo apt-get update && sudo apt-get install -y cpplint clang-format
else ifeq ($(UNAME_S), Darwin)
    INSTALL_CMD = brew install sdl2 sdl2_image && python3 -m pip install pillow
    DEV_INSTALL_CMD = brew install cpplint clang-format
else
    $(error Unsupported platform)
endif

SDL2_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)
SDL2_LFLAGS := $(shell $(SDL2_CONFIG) --libs) -lSDL2_image

ENGINE_DIRS = engine/core engine/math engine/platform engine/renderer engine/audio
GAME_DIRS = $(wildcard game/src/*)

ENGINE_SRC = $(foreach dir,$(ENGINE_DIRS),$(wildcard $(dir)/*.c))
GAME_SRC = $(foreach dir,$(GAME_DIRS),$(wildcard $(dir)/*.c))
HEADERS = $(foreach dir,$(ENGINE_DIRS) $(GAME_DIRS),$(wildcard $(dir)/*.h))
TEST_SRC = $(wildcard tests/*.c)

OBJ = $(ENGINE_SRC:.c=.o) $(GAME_SRC:.c=.o)
DEP = $(OBJ:.o=.d)
-include $(DEP)

INCLUDES = $(foreach dir,$(ENGINE_DIRS) $(GAME_DIRS),-I$(dir))
OPT ?= -O2
CFLAGS := -ggdb3 $(OPT) --std=c99 -Wall -Wextra -pedantic-errors -D_DEFAULT_SOURCE \
          $(INCLUDES) $(SDL2_CFLAGS)
LFLAGS := $(SDL2_LFLAGS) -lm

TARGET = super-fabio-bros
SPRITES = assets_cooked/sprites/manifest.txt

.PHONY: all assets install dev_install clean lint format test sanitize run snapshot

all: $(TARGET) assets

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LFLAGS)

# Cuts the generated sprite sheets in assets/source into transparent PNGs.
$(SPRITES): scripts/cook_sprites.py $(wildcard assets/source/*.jpeg)
	python3 scripts/cook_sprites.py

assets: $(SPRITES)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

install:
	$(INSTALL_CMD)

dev_install:
	$(DEV_INSTALL_CMD)

lint:
	cpplint --quiet --root=. $(ENGINE_SRC) $(GAME_SRC) $(HEADERS) $(TEST_SRC)

format:
	clang-format -i $(ENGINE_SRC) $(GAME_SRC) $(HEADERS) $(TEST_SRC)

test: assets
	python3 tests/run_tests.py

sanitize: assets
	python3 tests/run_tests.py --sanitize

run: all
	./$(TARGET)

# A frame of a bot playing for 20 seconds, and the title screen.
snapshot: all
	./$(TARGET) --seed 7 --snapshot snapshot.bmp --seconds 20
	./$(TARGET) --snapshot snapshot_title.bmp --title

clean:
	rm -f $(OBJ) $(DEP) $(TARGET) snapshot*.bmp
