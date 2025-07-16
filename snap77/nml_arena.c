#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "nml_mac.h"
#include "nml_arena.h"

static MAC_INLINE inline uintptr_t
arena_align(register uintptr_t ptr)
{
	const uintptr_t align = 2 * sizeof(uint8_t *);
	register uintptr_t mod = ptr & (align - 1);
	return mod != 0 ? ptr + (align - mod) : ptr;
}

static MAC_INLINE inline Region *
arena_append(Arena *arena, size_t size, size_t off, bool lock)
{
	size += arena_align(sizeof(Region));
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
	arena_append(arena, MAC_MIN(size, ARENA_PAGE), 0, false);
}

void
arena_reset(Arena *arena)
{
	Region *ptr = arena->head;
	while (ptr != NULL)
	{
		ptr->off = 0;
		ptr->lock = false;
		ptr = ptr->next;
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

MAC_HOT MAC_NORETNULL void *
arena_alloc(Arena *arena, size_t size)
{
	Region *ptr = arena->head;
	Region *prev = NULL;

	do
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
	while (ptr != NULL);

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

	do
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
	while (ptr != NULL);

	ptr = arena_append(arena, size, size, true);
	return ptr->mem;
}

MAC_HOT void
arena_unlock(Arena *arena, const void *mem)
{
	Region *ptr = arena->head;

	do
	{
		if (MAC_UNLIKELY(ptr->lock && mem == ptr->mem))
		{
			ptr->lock = false;
		}
		ptr = ptr->next;
	}
	while (ptr != NULL);
}

MAC_HOT void *
arena_update(Arena *arena, const void *mem, size_t size)
{
	Region *ptr = arena->head;
	Region *prev = NULL;

	do
	{
		if (MAC_UNLIKELY(ptr->lock && mem == ptr->mem))
		{
			if (size > ptr->size)
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
			else if (size == 0)
			{
				ptr->lock = false;
				ptr->off = 0;
				return NULL;
			}

			ptr->off = arena_align(size);
			return ptr->mem;
		}

		prev = ptr;
		ptr = ptr->next;
	}
	while (ptr != NULL);
	return (void *)mem;
}
