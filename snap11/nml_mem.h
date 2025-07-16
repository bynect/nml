#ifndef NML_MEM_H
#define NML_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nml_mac.h"

#define ARENA_MIN	128
#define ARENA_PAGE	4096

typedef struct Region {
	uint8_t *mem;
	size_t size;
	size_t off;
	bool lock;
	struct Region *next;
} Region;

typedef struct {
	Region *head;
	size_t size;
} Arena;

void arena_init(Arena *arena, size_t size);

void arena_shrink(Arena *arena, size_t size);

void arena_free(Arena *arena);

void arena_reset(Arena *arena, const void *mem);

MAC_HOT MAC_NORETNULL void *arena_alloc(Arena *arena, size_t size);

MAC_HOT MAC_NORETNULL void *arena_lock(Arena *arena, size_t size);

MAC_HOT void arena_unlock(Arena *arena, const void *mem);

MAC_HOT MAC_NORETNULL void *arena_update(Arena *arena, void *mem, size_t size);

#endif
