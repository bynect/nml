#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "nml_set.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_dbg.h"
#include "nml_str.h"

#define SET_IMPL(name, type, key, hashdata, hashlen, keycmp, 					\
				keydump, alloc, membase, memfactor, load)						\
																				\
	static MAC_HOT type ## _Set_Buck *											\
	name ## _find(type ## _Set_Buck *bucks, size_t size, type ## _Set_Key hk)	\
	{																			\
		type ## _Set_Buck *tomb = NULL;											\
		type ## _Set_Buck *buck;												\
																				\
		const size_t mask = size - 1;											\
		uint32_t i = hk.hsh & mask;												\
																				\
		while (true)															\
		{																		\
			buck = bucks + i;													\
			if (!buck->w_key)													\
			{																	\
				if (!buck->w_value)												\
				{																\
					return tomb != NULL ? tomb : buck;							\
				}																\
				else if (tomb == NULL)											\
				{																\
					tomb = buck;												\
				}																\
			}																	\
			else if (hk.hsh == buck->hk.hsh && keycmp(hk.k, buck->hk.k))		\
			{																	\
				return buck;													\
			}																	\
			i = (i + 1) & mask;													\
		}																		\
	}																			\
																				\
	static MAC_HOT inline void													\
	name ## _grow(type ## _Set *set, Arena *arena, size_t size)					\
	{																			\
		const size_t memsize = size * sizeof(type ## _Set_Buck);				\
		type ## _Set_Buck *bucks = alloc(arena, memsize);						\
		memset(bucks, 0, memsize);												\
																				\
		set->len = 0;															\
		for (size_t i = 0; i < set->size; ++i)									\
		{																		\
			type ## _Set_Buck *src = set->bucks + i;							\
			if (!src->w_key)													\
			{																	\
				continue;														\
			}																	\
																				\
			type ## _Set_Buck *dest = name ## _find(bucks, size, src->hk);		\
			memcpy(dest, src, sizeof(type ## _Set_Buck));						\
			++set->len;															\
		}																		\
																				\
		set->bucks = bucks;														\
		set->size = size;														\
	}																			\
																				\
	static MAC_HOT inline Set_Result											\
	name ## _insert_buck(type ## _Set *set, type ## _Set_Key hk, bool unique)	\
	{																			\
		type ## _Set_Buck *buck = name ## _find(set->bucks, set->size, hk);		\
		if (!buck->w_key && !buck->w_value)										\
		{																		\
			++set->len;															\
		}																		\
		else if (MAC_UNLIKELY(buck->w_key && unique))							\
		{																		\
			return SET_FAIL;													\
		}																		\
																				\
		buck->hk = hk;															\
		buck->w_key = true;														\
		buck->w_value = true;													\
		return SET_DONE;													\
	}																			\
																				\
	static MAC_HOT inline bool													\
	name ## _remove_buck(type ## _Set *set, type ## _Set_Key hk)				\
	{																			\
		type ## _Set_Buck *buck = name ## _find(set->bucks, set->size, hk);		\
		if (!buck->w_key)														\
		{																		\
			return false;														\
		}																		\
																				\
		buck->w_key = false;													\
		buck->w_value = true;													\
		return true;															\
	}																			\
																				\
	void																		\
	name ## _init(type ## _Set *set, Hash_Default seed)							\
	{																			\
		set->bucks = NULL;														\
		set->len = 0;															\
		set->size = 0;															\
		set->seed = seed;														\
	}																			\
																				\
	void																		\
	name ## _reset(type ## _Set *set)											\
	{																			\
		set->len = 0;															\
		memset(set->bucks, 0, set->size * sizeof(type ## _Set_Buck));			\
	}																			\
																				\
	MAC_CONST type ## _Set_Key													\
	name ## _key(const type ## _Set *set, const key k)							\
	{																			\
		return (type ## _Set_Key) {												\
			.k = k,																\
			.hsh = hash_default(hashdata(k), hashlen(k), set->seed),			\
		};																		\
	}																			\
																				\
	void																		\
	name ## _union(const type ## _Set *src, type ## _Set *dest, Arena *arena)	\
	{																			\
		MAC_PREFETCH_CACHE(src->bucks, 0);										\
		MAC_PREFETCH_CACHE(dest->bucks, 1);										\
																				\
		if (src->len > (dest->size - dest->len) * load)							\
		{																		\
			size_t size = MAC_UNLIKELY(dest->size == 0) ?						\
							membase : dest->size * memfactor;					\
																				\
			while (src->len > (size - dest->len) * load)						\
			{																	\
				size *= memfactor;												\
			}																	\
			name ## _grow(dest, arena, size);									\
		}																		\
																				\
		for (size_t i = 0; i < src->size; ++i)									\
		{																		\
			type ## _Set_Buck *buck = src->bucks + i;							\
			if (buck->w_key)													\
			{																	\
				name ## _insert_buck(dest, buck->hk, true);						\
			}																	\
		}																		\
	}																			\
																				\
	void																		\
	name ## _difference(const type ## _Set *set1, const type ## _Set *set2,		\
							type ## _Set *dest, Arena *arena)					\
	{																			\
		for (size_t i = 0; i < set1->size; ++i)									\
		{																		\
			type ## _Set_Buck *buck = set1->bucks + i;							\
			if (buck->w_key)													\
			{																	\
				if (name ## _lookup(set2, buck->hk.k))							\
				{																\
					continue;													\
				}																\
				name ## _insert(dest, arena, buck->hk.k);						\
			}																	\
		}																		\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _lookup(const type ## _Set *set, key k)								\
	{																			\
		type ## _Set_Key hk = name ## _key(set, k);								\
		type ## _Set_Buck *buck = name ## _find(set->bucks, set->size, hk);		\
		return buck->w_key;														\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _lookup_key(const type ## _Set *set, type ## _Set_Key hk)			\
	{																			\
		type ## _Set_Buck *buck = name ## _find(set->bucks, set->size, hk);		\
		return buck->w_key;														\
	}																			\
																				\
	MAC_HOT Set_Result															\
	name ## _insert(type ## _Set *set, Arena *arena, key k)						\
	{																			\
		MAC_PREFETCH_CACHE(set->bucks, 1);										\
		Set_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(set->len + 1 > set->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(set->size == 0) ?						\
							membase : set->size * memfactor;					\
			name ## _grow(set, arena, size);									\
			res |= SET_REALLOC;													\
		}																		\
																				\
		type ## _Set_Key hk = name ## _key(set, k);								\
		return res | name ## _insert_buck(set, hk, false);						\
	}																			\
																				\
	MAC_HOT Set_Result															\
	name ## _insert_key(type ## _Set *set, Arena *arena, type ## _Set_Key hk)	\
	{																			\
		MAC_PREFETCH_CACHE(set->bucks, 1);										\
		Set_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(set->len + 1 > set->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(set->size == 0) ?						\
							membase : set->size * memfactor;					\
			name ## _grow(set, arena, size);									\
			res |= SET_REALLOC;													\
		}																		\
																				\
		return res | name ## _insert_buck(set, hk, false);						\
	}																			\
																				\
	MAC_HOT Set_Result															\
	name ## _unique(type ## _Set *set, Arena *arena, key k)						\
	{																			\
		MAC_PREFETCH_CACHE(set->bucks, 1);										\
		Set_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(set->len + 1 > set->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(set->size == 0) ?						\
							membase : set->size * memfactor;					\
			name ## _grow(set, arena, size);									\
			res |= SET_REALLOC;													\
		}																		\
																				\
		type ## _Set_Key hk = name ## _key(set, k);								\
		return res | name ## _insert_buck(set, hk, true);						\
	}																			\
																				\
	MAC_HOT Set_Result															\
	name ## _unique_key(type ## _Set *set, Arena *arena, type ## _Set_Key hk)	\
	{																			\
		MAC_PREFETCH_CACHE(set->bucks, 1);										\
		Set_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(set->len + 1 > set->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(set->size == 0) ?						\
							membase : set->size * memfactor;					\
			name ## _grow(set, arena, size);									\
			res |= SET_REALLOC;													\
		}																		\
																				\
		return res | name ## _insert_buck(set, hk, true);						\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _remove(type ## _Set *set, key k)									\
	{																			\
		if (MAC_UNLIKELY(set->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(set->bucks, 1);										\
		type ## _Set_Key hk = name ## _key(set, k);								\
		return name ## _remove_buck(set, hk);									\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _remove_key(type ## _Set *set, type ## _Set_Key hk)					\
	{																			\
		if (MAC_UNLIKELY(set->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(set->bucks, 1);										\
		return name ## _remove_buck(set, hk);									\
	}																			\
																				\
	void																		\
	name ## _iter(type ## _Set *set, type ## _Set_Iter_Fn fn, void *ctx)		\
	{																			\
		for (size_t i = 0; i < set->size; ++i)									\
		{																		\
			type ## _Set_Buck *buck = set->bucks + i;							\
			fn(buck->w_key ? buck : NULL, ctx);									\
		}																		\
	}																			\
																				\
	void																		\
	name ## _iter_dump(type ## _Set_Buck *buck, void *ctx)						\
	{																			\
		MAC_UNUSED(ctx);														\
		if (buck != NULL)														\
		{																		\
			fprintf(stdout, "Key hash: ");										\
			dbg_dump_hash_default(buck->hk.hsh);								\
																				\
			fprintf(stdout, "Key dump: ");										\
			keydump(buck->hk.k);												\
		}																		\
	}


#define SET_ALLOC(arena, size)	arena_alloc(arena, size)
#define SET_LOAD				0.75

#define SET_STRDATA(key)		(void *)key.str
#define SET_STRLEN(key)			key.len

SET_IMPL(str_set, Str, Sized_Str,
		SET_STRDATA, SET_STRLEN, STR_EQUAL,
		dbg_dump_sized_str,
		SET_ALLOC, 32, 2, SET_LOAD)
