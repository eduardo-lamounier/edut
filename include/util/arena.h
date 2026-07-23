#ifndef ARENA_H
#define ARENA_H

#include<stdlib.h>

#define KB(x) ((x) * (size_t) 1024)
#define MB(x) (KB(x) * (size_t) 1024)

typedef struct arena arena_t;

arena_t *arena_new(size_t capacity);

void arena_destroy(arena_t *arena);

void arena_reset(arena_t *arena);

void *arena_alloc(arena_t *arena, size_t n, size_t size);

#endif
