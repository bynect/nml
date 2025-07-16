#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nml_tab.h"
#include "nml_expr.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_arena.h"
#include "nml_dump.h"
#include "nml_type.h"
#include "nml_str.h"
#include "nml_check.h"
#include "nml_comp.h"

#define TAB_IMPL(name, type, key, value, hashdata, hashlen, keycmp, 			\
				keydump, valuedump, alloc, free, membase, memfactor, load)		\
																				\
	static MAC_HOT type ## _Tab_Buck *											\
	name ## _find(type ## _Tab_Buck *bucks, size_t size, type ## _Tab_Key hk)	\
	{																			\
		const size_t mask = size - 1;											\
		register size_t i = hk.hsh & mask;										\
		type ## _Tab_Buck *tomb = NULL;											\
																				\
		while (true)															\
		{																		\
			type ## _Tab_Buck *buck = bucks + i;								\
			if (!buck->set)														\
			{																	\
				if (!buck->tomb)												\
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
	name ## _grow(type ## _Tab *tab, size_t size)								\
	{																			\
		const size_t memsize = size * sizeof(type ## _Tab_Buck);				\
		type ## _Tab_Buck *bucks = alloc(memsize);								\
		memset(bucks, 0, memsize);												\
																				\
		tab->len = 0;															\
		for (size_t i = 0; i < tab->size; ++i)									\
		{																		\
			type ## _Tab_Buck *src = tab->bucks + i;							\
			if (!src->set)														\
			{																	\
				continue;														\
			}																	\
																				\
			type ## _Tab_Buck *dest = name ## _find(bucks, size, src->hk);		\
			memcpy(dest, src, sizeof(type ## _Tab_Buck));						\
			++tab->len;															\
		}																		\
																				\
		free(tab->bucks);														\
		tab->bucks = bucks;														\
		tab->size = size;														\
	}																			\
																				\
	static MAC_HOT inline bool													\
	name ## _get_buck(const type ## _Tab *tab, type ## _Tab_Key hk, value *v)	\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		if (!buck->set)															\
		{																		\
			return false;														\
		}																		\
																				\
		*v = buck->v;															\
		return true;															\
	}																			\
																				\
	static MAC_HOT inline Tab_Result											\
	name ## _insert_buck(type ## _Tab *tab, type ## _Tab_Key hk,				\
						value v, bool unique)									\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		if (!buck->set && !buck->tomb)											\
		{																		\
			++tab->len;															\
		}																		\
		else if (MAC_UNLIKELY(buck->set && unique))								\
		{																		\
			return TAB_FAIL;													\
		}																		\
																				\
		buck->hk = hk;															\
		buck->v = v;															\
		buck->set = true;														\
		buck->tomb = false;														\
		return TAB_DONE;														\
	}																			\
																				\
	static MAC_HOT inline bool													\
	name ## _remove_buck(type ## _Tab *tab, type ## _Tab_Key hk)				\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		if (!buck->set)															\
		{																		\
			return false;														\
		}																		\
																				\
		buck->set = false;														\
		buck->tomb = true;														\
		return true;															\
	}																			\
																				\
	static MAC_HOT inline bool													\
	name ## _pop_buck(type ## _Tab *tab, type ## _Tab_Key hk, value *v)			\
	{																			\
		type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);		\
		if (!buck->set)															\
		{																		\
			return false;														\
		}																		\
																				\
		buck->set = false;														\
		buck->tomb = true;														\
		*v = buck->v;															\
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
	name ## _free(type ## _Tab *tab)											\
	{																			\
		if (tab->bucks != NULL)													\
		{																		\
			free(tab->bucks);													\
		}																		\
	}																			\
																				\
	type ## _Tab_Key															\
	name ## _key(const type ## _Tab *tab, const key k)							\
	{																			\
		return (type ## _Tab_Key) {												\
			.k = k,																\
			.hsh = hash_default(hashdata(k), hashlen(k), tab->seed),			\
		};																		\
	}																			\
																				\
	void																		\
	name ## _copy(const type ## _Tab *MAC_RESTRICT tab,							\
					type ## _Tab *MAC_RESTRICT dest)							\
	{																			\
		const size_t memsize = tab->size * sizeof(type ## _Tab_Buck);			\
		dest->bucks = realloc(dest->bucks, memsize);							\
		memcpy(dest->bucks, tab->bucks, memsize);								\
																				\
		dest->size = tab->size;													\
		dest->len = tab->len;													\
		dest->seed = tab->seed;													\
	}																			\
																				\
	void																		\
	name ## _union(const type ## _Tab *MAC_RESTRICT tab,						\
					type ## _Tab *MAC_RESTRICT dest)							\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 0);										\
		MAC_PREFETCH_CACHE(dest->bucks, 1);										\
																				\
		if (tab->len > (dest->size - dest->len) * load)							\
		{																		\
			size_t size = MAC_UNLIKELY(dest->size == 0) ?						\
							membase : dest->size * memfactor;					\
																				\
			while (tab->len > (size - dest->len) * load)						\
			{																	\
				size *= memfactor;												\
			}																	\
			name ## _grow(dest, size);											\
		}																		\
																				\
		for (size_t i = 0; i < tab->size; ++i)									\
		{																		\
			type ## _Tab_Buck *buck = tab->bucks + i;							\
			if (buck->set)														\
			{																	\
				type ## _Tab_Key hk = name ## _key(tab, buck->hk.k);			\
				name ## _insert_buck(dest, hk, buck->v, true);					\
			}																	\
		}																		\
	}																			\
																				\
	void																		\
	name ## _union_key(const type ## _Tab *MAC_RESTRICT tab,					\
						type ## _Tab *MAC_RESTRICT dest)						\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 0);										\
		MAC_PREFETCH_CACHE(dest->bucks, 1);										\
																				\
		if (tab->len > (dest->size - dest->len) * load)							\
		{																		\
			size_t size = MAC_UNLIKELY(dest->size == 0) ?						\
							membase : dest->size * memfactor;					\
																				\
			while (tab->len > (size - dest->len) * load)						\
			{																	\
				size *= memfactor;												\
			}																	\
			name ## _grow(dest, size);											\
		}																		\
																				\
		for (size_t i = 0; i < tab->size; ++i)									\
		{																		\
			type ## _Tab_Buck *buck = tab->bucks + i;							\
			if (buck->set)														\
			{																	\
				name ## _insert_buck(dest, buck->hk, buck->v, true);			\
			}																	\
		}																		\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _member(const type ## _Tab *tab, key k)								\
	{																			\
		if (MAC_LIKELY(tab->size != 0))											\
		{																		\
			MAC_PREFETCH_CACHE(tab->bucks, 0);									\
			type ## _Tab_Key hk = name ## _key(tab, k);							\
			type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);	\
			return buck->set;													\
		}																		\
		return false;															\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _member_key(const type ## _Tab *tab, type ## _Tab_Key hk)			\
	{																			\
		if (MAC_LIKELY(tab->size != 0))											\
		{																		\
			MAC_PREFETCH_CACHE(tab->bucks, 0);									\
			type ## _Tab_Buck *buck = name ## _find(tab->bucks, tab->size, hk);	\
			return buck->set;													\
		}																		\
		return false;															\
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
	MAC_HOT Tab_Result															\
	name ## _insert(type ## _Tab *tab, key k, value v)							\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		Tab_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, size);											\
			res |= TAB_REALLOC;													\
		}																		\
																				\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		return res | name ## _insert_buck(tab, hk, v, false);					\
	}																			\
																				\
	MAC_HOT Tab_Result															\
	name ## _insert_key(type ## _Tab *tab, type ## _Tab_Key hk, value v)		\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		Tab_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, size);											\
			res |= TAB_REALLOC;													\
		}																		\
																				\
		return res | name ## _insert_buck(tab, hk, v, false);					\
	}																			\
																				\
	MAC_HOT Tab_Result															\
	name ## _unique(type ## _Tab *tab, key k, value v)							\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		Tab_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, size);											\
			res |= TAB_REALLOC;													\
		}																		\
																				\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		return res | name ## _insert_buck(tab, hk, v, true);					\
	}																			\
																				\
	MAC_HOT Tab_Result															\
	name ## _unique_key(type ## _Tab *tab, type ## _Tab_Key hk, value v)		\
	{																			\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		Tab_Result res = 0;														\
																				\
		if (MAC_UNLIKELY(tab->len + 1 > tab->size * load))						\
		{																		\
			size_t size = MAC_UNLIKELY(tab->size == 0) ?						\
							membase : tab->size * memfactor;					\
			name ## _grow(tab, size);											\
			res |= TAB_REALLOC;													\
		}																		\
																				\
		return res | name ## _insert_buck(tab, hk, v, true);					\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _remove(type ## _Tab *tab, key k)									\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		return name ## _remove_buck(tab, hk);									\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _remove_key(type ## _Tab *tab, type ## _Tab_Key hk)					\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		return name ## _remove_buck(tab, hk);									\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _pop(type ## _Tab *tab, key k, value *v)							\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		type ## _Tab_Key hk = name ## _key(tab, k);								\
		return name ## _pop_buck(tab, hk, v);									\
	}																			\
																				\
	MAC_HOT bool																\
	name ## _pop_key(type ## _Tab *tab, type ## _Tab_Key hk, value *v)			\
	{																			\
		if (MAC_UNLIKELY(tab->bucks == NULL))									\
		{																		\
			return false;														\
		}																		\
																				\
		MAC_PREFETCH_CACHE(tab->bucks, 1);										\
		return name ## _pop_buck(tab, hk, v);									\
	}																			\
	void																		\
	name ## _iter(type ## _Tab *tab, type ## _Tab_Iter_Fn fn, void *ctx)		\
	{																			\
		for (size_t i = 0; i < tab->size; ++i)									\
		{																		\
			type ## _Tab_Buck *buck = tab->bucks + i;							\
			fn(buck->set ? buck : NULL, ctx);									\
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
			dump_hash_default(buck->hk.hsh);									\
																				\
			fprintf(stdout, "Key dump: ");										\
			keydump(buck->hk.k);												\
																				\
			fprintf(stdout, "Value dump: ");									\
			valuedump(buck->v);													\
			fprintf(stdout, "\n");												\
		}																		\
	}

#define TAB_FACTOR				2
#define TAB_LOAD				0.75

#define TAB_CMP(a, b)			(a == b)
#define TAB_NOP(...)

#define TAB_STRDATA(key)		(void *)key.str
#define TAB_INTDATA(key)		(void *)&key

#define TAB_STRSIZE(key)		key.len
#define TAB_INTSIZE(key)		sizeof(key)

#define TAB_INTDUMP(value)		fprintf(stdout, "%u\n", value)
#define TAB_EXPRDUMP(value)		dump_expr_node(value, 0, true)
#define TAB_SYMDUMP(value)		dump_symbol(&value)
#define TAB_VALUEDUMP(value)	dump_ir_value(&value); fputc('\n', stdout)

TAB_IMPL(str_tab, Str, Sized_Str, Sized_Str,
		TAB_STRDATA, TAB_STRSIZE, STR_EQUAL,
		dump_sized_str, dump_sized_str,
		malloc, free, 32, TAB_FACTOR, TAB_LOAD)

TAB_IMPL(id_tab, Id, Sized_Str, uint32_t,
		TAB_STRDATA, TAB_STRSIZE, STR_EQUAL,
		dump_sized_str, TAB_INTDUMP,
		malloc, free, 32, TAB_FACTOR, TAB_LOAD)

TAB_IMPL(type_tab, Type, Sized_Str, Type,
		TAB_STRDATA, TAB_STRSIZE, STR_EQUAL,
		dump_sized_str, dump_type,
		malloc, free, 32, TAB_FACTOR, TAB_LOAD)

TAB_IMPL(expr_tab, Expr, Sized_Str, Expr_Node *,
		TAB_STRDATA, TAB_STRSIZE, STR_EQUAL,
		dump_sized_str, TAB_EXPRDUMP,
		malloc, free, 32, TAB_FACTOR, TAB_LOAD)

//TAB_IMPL(sym_tab, Sym, Sized_Str, Symbol,
//		TAB_STRDATA, TAB_STRSIZE, STR_EQUAL,
//		dump_sized_str, TAB_SYMDUMP,
//		arena_alloc, 32, TAB_FACTOR, TAB_LOAD)

TAB_IMPL(scheme_tab, Scheme, Sized_Str, Type_Scheme,
		TAB_STRDATA, TAB_STRSIZE, STR_EQUAL,
		dump_sized_str, dump_type_scheme,
		malloc, free, 32, TAB_FACTOR, TAB_LOAD)
