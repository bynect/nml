#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "nml_ast.h"
#include "nml_hash.h"
#include "nml_tab.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_dbg.h"
#include "nml_type.h"
#include "nml_str.h"
#include "nml_fold.h"

#define TAB_IMPL(name, type, key, value, hashdata, hashlen,	keycmp, 			\
				keydump, valuedump, alloc, membase, memfactor, load)			\
																				\
	static MAC_HOT type ## _Tab_Buck *											\
	name ## _find(type ## _Tab_Buck *bucks, size_t size, type ## _Tab_Key hk)	\
	{																			\
		type ## _Tab_Buck *tomb = NULL;											\
		type ## _Tab_Buck *buck;												\
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
			else if (hk.hsh == buck->hk.hsh &&									\
					keycmp(hk.k, buck->hk.k))									\
			{																	\
				return buck;													\
			}																	\
			i = (i + 1) & mask;													\
		}																		\
	}																			\
																				\
	static MAC_INLINE inline void												\
	name ## _grow(type ## _Tab *tab, Arena *arena, size_t size)					\
	{																			\
		const size_t memsize = size * sizeof(type ## _Tab_Buck);				\
		type ## _Tab_Buck *bucks = alloc(arena, memsize);						\
		memset(bucks, 0, memsize);												\
																				\
		tab->len = 0;															\
		for (size_t i = 0; i < tab->size; ++i)									\
		{																		\
			type ## _Tab_Buck *src = tab->bucks + i;							\
			if (!src->w_key)													\
			{																	\
				continue;														\
			}																	\
																				\
			type ## _Tab_Buck *dest = name ## _find(bucks, size, src->hk);		\
			memcpy(dest, src, sizeof(type ## _Tab_Buck));						\
			++tab->len;															\
		}																		\
																				\
		tab->bucks = bucks;														\
		tab->size = size;														\
	}																			\
																				\
	static MAC_INLINE inline bool												\
	name ## _get_buck(const type ## _Tab *tab, type ## _Tab_Key hk, value *v)	\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		if (!buck->w_key)														\
		{																		\
			return false;														\
		}																		\
																				\
		*v = buck->v;															\
		return true;															\
	}																			\
																				\
	static MAC_INLINE inline void												\
	name ## _set_buck(type ## _Tab *tab, type ## _Tab_Key hk,					\
						value v, bool unique)									\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		if (!buck->w_key && !buck->w_value)										\
		{																		\
			++tab->len;															\
		}																		\
		else if (MAC_UNLIKELY(buck->w_key && unique))							\
		{																		\
			return;																\
		}																		\
																				\
		buck->hk = hk;															\
		buck->v = v;															\
		buck->w_key = true;														\
		buck->w_value = true;													\
	}																			\
																				\
	static MAC_INLINE inline bool												\
	name ## _del_buck(type ## _Tab *tab, type ## _Tab_Key hk)					\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
																				\
		if (!buck->w_key)														\
		{																		\
			return false;														\
		}																		\
		buck->w_key = false;													\
		buck->w_value = true;													\
		return true;															\
	}																			\
																				\
	void																		\
	name ## _init(type ## _Tab *tab, Hash_Default seed)							\
	{																			\
		tab->bucks = NULL;														\
		tab->len = 0;															\
		tab->size = 0;															\
		tab->seed = seed;														\
	}																			\
																				\
	void																		\
	name ## _reset(type ## _Tab *tab)											\
	{																			\
		tab->len = 0;															\
		memset(tab->bucks, 0, tab->size * sizeof(type ## _Tab_Buck));			\
	}																			\
																				\
	void																		\
	name ## _merge(const type ## _Tab *src, type ## _Tab *dest, Arena *arena)	\
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
			type ## _Tab_Buck *buck = src->bucks + i;							\
			if (buck->w_key)													\
			{																	\
				name ## _set_buck(dest, buck->hk, buck->v, true);				\
			}																	\
		}																		\
	}																			\
																				\
	type ## _Tab_Key															\
	name ## _key(const type ## _Tab *tab, const key k)							\
	{																			\
		return (type ## _Tab_Key) {												\
			.k = k,																\
			.hsh = HASH_DEFAULT(hashdata(k), hashlen(k), tab->seed),			\
		};																		\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _lookup(const type ## _Tab *tab, key k)								\
	{																			\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		return buck->w_key;														\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _lookup_key(const type ## _Tab *tab, type ## _Tab_Key hk)			\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		return buck->w_key;														\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _get(const type ## _Tab *tab, key k, value *v)						\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 0);										\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		return name ## _get_buck(tab, hk, v);									\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _get_key(const type ## _Tab *tab, type ## _Tab_Key hk, value *v)	\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 0);										\
		return name ## _get_buck(tab, hk, v);									\
	}																			\
																				\
	MAC_HOT void																\
	name ## _set(type ## _Tab *tab, Arena *arena, key k, value v)				\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, arena, size);									\
		}																		\
																				\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		name ## _set_buck(tab, hk, v, false);									\
	}																			\
																				\
	MAC_HOT void																\
	name ## _set_key(type ## _Tab *tab, Arena *arena,							\
					type ## _Tab_Key hk, value v)								\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, arena, size);									\
		}																		\
																				\
		name ## _set_buck(tab, hk, v, false);									\
	}																			\
																				\
	MAC_HOT void																\
	name ## _unique(type ## _Tab *tab, Arena *arena, key k, value v)			\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, arena, size);									\
		}																		\
																				\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		name ## _set_buck(tab, hk, v, true);									\
	}																			\
																				\
	MAC_HOT void																\
	name ## _unique_key(type ## _Tab *tab, Arena *arena,						\
					type ## _Tab_Key hk, value v)								\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, arena, size);									\
		}																		\
																				\
		name ## _set_buck(tab, hk, v, true);									\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _del(type ## _Tab *tab, key k)										\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		return name ## _del_buck(tab, hk);										\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _del_key(type ## _Tab *tab, type ## _Tab_Key hk)					\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		return name ## _del_buck(tab, hk);										\
	}																			\
																				\
	void																		\
	name ## _iter(type ## _Tab *tab, type ## _Tab_Iter_Fn fn, void *ctx)		\
	{																			\
		for (size_t i = 0; i < tab->size; ++i)									\
		{																		\
			type ## _Tab_Buck *buck = tab->bucks + i;							\
			fn(buck->w_key ? buck : NULL, ctx);									\
		}																		\
	}																			\
																				\
	void																		\
	name ## _iter_dump(type ## _Tab_Buck *buck, void *ctx)						\
	{																			\
		MAC_UNUSED(ctx);														\
		if (buck != NULL)														\
		{																		\
			fprintf(stdout, "Key hash: ");										\
			dbg_dump_hash_default(buck->hk.hsh);								\
																				\
			fprintf(stdout, "Key dump: ");										\
			keydump(buck->hk.k);												\
																				\
			fprintf(stdout, "Value dump: ");									\
			valuedump(buck->v);													\
			fprintf(stdout, "\n");												\
		}																		\
	}																			\

#define TAB_ALLOC(arena, size)	arena_alloc(arena, size)
#define TAB_LOAD				0.75

#define TAB_STRDATA(key)		(void *)key.str
#define TAB_STRLEN(key)			key.len
#define TAB_STRCMP(k1, k2)		STR_EQUAL(k1, k2)

#define TAB_STRDUMP(key)		dbg_dump_sized_str(key)
#define TAB_TYPEDUMP(value)		dbg_dump_type(value)
#define TAB_CONSTDUMP(value)													\
	do																			\
	{																			\
		fprintf(stdout, "%s", value.dead ? "dead" : "used");					\
		dbg_dump_ast_node(value.node, 0, true);									\
	}																			\
	while (false)																\

#define TAB_INTDATA(key)		(void *)&key
#define TAB_INTSIZE(key)		sizeof(Type_Id)
#define TAB_INTCMP(k1, k2)		(k1 == k2)
#define TAB_INTDUMP(value)		fprintf(stdout, "%u\n", value)

TAB_IMPL(str_tab, Str, Sized_Str, Sized_Str,
		TAB_STRDATA, TAB_STRLEN, TAB_STRCMP,
		TAB_STRDUMP, TAB_STRDUMP,
		TAB_ALLOC, 32, 2, TAB_LOAD)

TAB_IMPL(type_tab, Type, Sized_Str, Type,
		TAB_STRDATA, TAB_STRLEN, TAB_STRCMP,
		TAB_STRDUMP, TAB_TYPEDUMP,
		TAB_ALLOC, 32, 2, TAB_LOAD)

TAB_IMPL(subst_tab, Subst, Type_Id, Type,
		TAB_INTDATA, TAB_INTSIZE, TAB_INTCMP,
		TAB_INTDUMP, TAB_TYPEDUMP,
		TAB_ALLOC, 32, 2, TAB_LOAD)

TAB_IMPL(fold_tab, Fold, Sized_Str, Fold_Node,
		TAB_STRDATA, TAB_STRLEN, TAB_STRCMP,
		TAB_STRDUMP, TAB_CONSTDUMP,
		TAB_ALLOC, 32, 2, TAB_LOAD)
