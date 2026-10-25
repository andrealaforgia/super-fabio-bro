#include <stdio.h>
#include <string.h>

#include "test_groups.h"

typedef struct {
  const char* name;
  void (*run)(void);
} test_group_t;

static const test_group_t groups[] = {
          {"level", run_level_tests},
    {"physics", run_physics_tests}, {"player", run_player_tests},
    {"enemy", run_enemy_tests},     {"combat", run_combat_tests},
    {"book_reader", run_book_reader_tests}, {"bot", run_bot_tests},         {"scenery", run_scenery_tests},
    {"sprites", run_sprites_tests}, {"audio", run_audio_tests},
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: tests <group>\n");
    return 2;
  }
  for (size_t i = 0; i < sizeof(groups) / sizeof(groups[0]); i++) {
    if (strcmp(groups[i].name, argv[1]) == 0) {
      groups[i].run();
      return 0;
    }
  }
  fprintf(stderr, "unknown test group %s\n", argv[1]);
  return 2;
}
