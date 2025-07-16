#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "nml_mac.h"
#include "nml_mem.h"

static MAC_INLINE inline uintptr_t
arena_align(uintptr_t ptr)
{
	const uintptr_t align = 2 * sizeof(uint8_t *);
	uintptr_t mod = ptr & (align - 1);
	uintptr_t x =  mod != 0 ? ptr + (align - mod) : ptr;
	return x;
}

static MAC_INLINE inline size_t
arena_max(size_t a, size_t b)
{
	return a > b ? a : b;
}

static MAC_INLINE inline Region *
arena_append(Arena *arena, size_t size, size_t off, bool lock)
{
	size = arena_align(size + sizeof(Region));
	Region *ptr = malloc(size);

	const uintptr_t base = (uintptr_t)ptr;
	const uintptr_t mem = arena_align(base + sizeof(Region));
	ptr->mem = (uint8_t *)mem;
	ptr->size = size - (mem - base);

	ptr->off = arena_align(mem + off) - mem;
	ptr->lock = lock;

	ptr->next = arena->head;
	arena->head = ptr;
	return ptr;
}

void
arena_init(Arena *arena, size_t size)
{
	arena->head = NULL;
	arena->size = size;
	arena_append(arena, arena_max(size, ARENA_MIN), 0, false);
}

void
arena_shrink(Arena *arena, size_t size)
{
	Region *ptr = arena->head;
	Region *prev = ptr;

	size_t curr = 0;
	while (ptr != NULL && curr < size)
	{
		curr += ptr->size;
		prev = ptr;
		ptr = ptr->next;
	}

	if (curr > size)
	{
		prev->next = NULL;
		while (ptr != NULL)
		{
			Region *next = ptr->next;
			free(ptr);
			ptr = next;
		}
	}
}

void
arena_free(Arena *arena)
{
	Region *ptr = arena->head;
	while (ptr != NULL)
	{
		Region *next = ptr->next;
		free(ptr);
		ptr = next;
	}
}

void
arena_reset(Arena *arena, const void *mem)
{
	Region *ptr = arena->head;
	if (mem == NULL)
	{
		while (ptr != NULL)
		{
			ptr->off = 0;
			ptr->lock = false;
			ptr = ptr->next;
		}
	}
	else
	{
		while (ptr != NULL)
		{
			if (MAC_UNLIKELY(ptr->lock && mem == ptr->mem))
			{
				ptr->lock = false;
				ptr->off = 0;
			}
			ptr = ptr->next;
		}
	}
}

MAC_HOT MAC_NORETNULL void *
arena_alloc(Arena *arena, size_t size)
{
	Region *ptr = arena->head;
	Region *prev = NULL;

	while (ptr != NULL)
	{
		if (!ptr->lock)
		{
			const uintptr_t base = (uintptr_t)ptr->mem;
			uintptr_t curr = base + (uintptr_t)ptr->off;
			uintptr_t off = arena_align(curr) - base;

			if (off + size <= ptr->size)
			{
				ptr->off = off + size;
				return ptr->mem + off;
			}
			else if (MAC_UNLIKELY(ptr->off == 0))
			{
				size = size < arena->size ? arena->size :
							size < ARENA_PAGE ? ARENA_PAGE : size;
				size = arena_align(size + sizeof(Region));

				if (MAC_UNLIKELY(ptr == arena->head))
				{
					arena->head = realloc(ptr, size);
					ptr = arena->head;
				}
				else
				{
					ptr = realloc(ptr, size);
				}

				if (MAC_LIKELY(prev != NULL))
				{
					prev->next = ptr;
				}

				const uintptr_t base = (uintptr_t)ptr;
				const uintptr_t mem = arena_align(base + sizeof(Region));
				ptr->mem = (uint8_t *)mem;
				ptr->size = size - (mem - base);

				ptr->off = arena_align(size);
				return ptr->mem;
			}
		}

		prev = ptr;
		ptr = ptr->next;
	}

	size_t off = size;
	size = size < arena->size ? arena->size :
				size < ARENA_PAGE ? ARENA_PAGE : size;

	ptr = arena_append(arena, size, off, false);
	return ptr->mem;
}

MAC_HOT MAC_NORETNULL void *
arena_lock(Arena *arena, size_t size)
{
	Region *ptr = arena->head;
	Region *prev = NULL;

	while (ptr != NULL)
	{
		if (MAC_UNLIKELY(ptr->off == 0 && !ptr->lock))
		{
			if (MAC_UNLIKELY(size > ptr->size))
			{
				size = arena_align(size + sizeof(Region));

				if (MAC_UNLIKELY(ptr == arena->head))
				{
					arena->head = realloc(ptr, size);
					ptr = arena->head;
				}
				else
				{
					ptr = realloc(ptr, size);
				}

				if (MAC_LIKELY(prev != NULL))
				{
					prev->next = ptr;
				}

				const uintptr_t base = (uintptr_t)ptr;
				const uintptr_t mem = arena_align(base + sizeof(Region));
				ptr->mem = (uint8_t *)mem;
				ptr->size = size - (mem - base);
			}

			ptr->lock = true;
			ptr->off = size;
			return ptr->mem;
		}

		prev = ptr;
		ptr = ptr->next;
	}

	ptr = arena_append(arena, size, size, true);
	return ptr->mem;
}

MAC_HOT void
arena_unlock(Arena *arena, const void *mem)
{
	Region *ptr = arena->head;

	while (ptr != NULL)
	{
		if (MAC_UNLIKELY(ptr->lock && mem == ptr->mem))
		{
			ptr->lock = false;
		}
		ptr = ptr->next;
	}
}

MAC_HOT void *
arena_update(Arena *arena, void *mem, size_t size)
{
	Region *ptr = arena->head;
	Region *prev = NULL;

	while (ptr != NULL)
	{
		if (MAC_UNLIKELY(ptr->lock && mem == ptr->mem))
		{
			if (MAC_LIKELY(size > ptr->size))
			{
				size = arena_align((size < ARENA_MIN ? ARENA_MIN : size) + sizeof(Region));

				if (MAC_UNLIKELY(ptr == arena->head))
				{
					arena->head = realloc(ptr, size);
					ptr = arena->head;
				}
				else
				{
					ptr = realloc(ptr, size);
				}

				if (MAC_LIKELY(prev != NULL))
				{
					prev->next = ptr;
				}

				const uintptr_t base = (uintptr_t)ptr;
				const uintptr_t mem = arena_align(base + sizeof(Region));
				ptr->mem = (uint8_t *)mem;
				ptr->size = size - (mem - base);
			}
			ptr->off = arena_align(size);
			return ptr->mem;
		}

		prev = ptr;
		ptr = ptr->next;
	}
	return mem;
}
