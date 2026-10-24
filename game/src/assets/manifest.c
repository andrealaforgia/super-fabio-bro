#include "manifest.h"

#include <stdio.h>
#include <string.h>

int manifest_parse(const char* text, manifest_entry_t* entries, int capacity) {
  int count = 0;
  const char* line = text;
  while (*line) {
    const char* end = strchr(line, '\n');
    size_t length = end ? (size_t)(end - line) : strlen(line);
    if (length > 0) {
      char buffer[256];
      if (length >= sizeof(buffer)) return -1;
      memcpy(buffer, line, length);
      buffer[length] = '\0';
      if (count >= capacity) return -1;
      manifest_entry_t* e = &entries[count];
      char name[sizeof(e->name)], file[sizeof(e->file)];
      if (sscanf(buffer, "%39s %47s %d %d %d %d", name, file, &e->w, &e->h,
                 &e->anchor_x, &e->anchor_y) != 6)
        return -1;
      if (e->w <= 0 || e->h <= 0) return -1;
      strcpy(e->name, name);
      strcpy(e->file, file);
      count++;
    }
    if (!end) break;
    line = end + 1;
  }
  return count;
}

const manifest_entry_t* manifest_find(const manifest_entry_t* entries,
                                      int count, const char* name) {
  for (int i = 0; i < count; i++)
    if (strcmp(entries[i].name, name) == 0) return &entries[i];
  return NULL;
}
