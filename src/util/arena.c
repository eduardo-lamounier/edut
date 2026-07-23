#include "util/arena.h"

struct arena {
  char *data;
  size_t capacity;
  size_t offset;
};

arena_t *arena_new(size_t capacity) {
  arena_t *arena = calloc(1, sizeof(arena_t));

  if(arena == NULL)
    abort();

  arena->capacity = capacity; 
  arena->data = calloc(capacity, 1);

  if(arena->data == NULL) {
    free(arena);
    abort();
  }

  return arena;
}


void arena_destroy(arena_t *arena) {
  free(arena->data);
  free(arena);
}

void arena_reset(arena_t *arena) {
  arena->offset = 0;
}

void *arena_alloc(arena_t *arena, size_t n, size_t size) {
  arena->offset = (arena->offset + 8-1) & ~(8-1);

  void *addr = arena->data + arena->offset;

  arena->offset += n * size;

  if(arena->offset >= arena->capacity)
    abort();
  
  return addr;
}

