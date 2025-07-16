#ifndef NML_TAB_H
#define NML_TAB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_hash.h"
#include "nml_str.h"

#define TAB_DECL(name, type, key, value) 												\
	typedef struct {																	\
		key k;																			\
		Hash_Default hsh;																\
	} type ## _Tab_Key;																	\
																						\
	typedef struct {																	\
		type ## _Tab_Key hk;															\
		value v;																		\
		bool w_key;																		\
		bool w_value;																	\
	} type ## _Tab_Buck;																\
																						\
	typedef struct {																	\
		type ## _Tab_Buck *bucks;														\
		size_t len;																		\
		size_t size;																	\
		Hash_Default seed;																\
	} type ## _Tab;																		\
																						\
	typedef void (*type ## _Tab_Iter_Fn)(type ## _Tab_Buck *buck, void *ctx);			\
																						\
	void name ## _init(type ## _Tab *tab, Hash_Default seed);							\
																						\
	void name ## _reset(type ## _Tab *tab);												\
																						\
	void name ## _merge(const type ## _Tab *src, type ## _Tab *dest, Arena *arena);		\
																						\
	type ## _Tab_Key name ## _key(const type ## _Tab *tab, const key k);				\
																						\
	MAC_HOT bool name ## _lookup(const type ## _Tab *tab, key k);						\
																						\
	MAC_HOT bool name ## _lookup_key(const type ## _Tab *tab, type ## _Tab_Key hk);		\
																						\
	MAC_HOT bool name ## _get(const type ## _Tab *tab, key k, value *v);				\
																						\
	MAC_HOT bool name ## _get_key(const type ## _Tab *tab,								\
									type ## _Tab_Key hk, value *v);						\
																						\
	MAC_HOT void name ## _set(type ## _Tab *tab, Arena *arena, key k, value v);			\
																						\
	MAC_HOT void name ## _set_key(type ## _Tab *tab, Arena *arena,						\
								type ## _Tab_Key hk, value v);							\
																						\
	MAC_HOT void name ## _unique(type ## _Tab *tab, Arena *arena, key k, value v);		\
																						\
	MAC_HOT void name ## _unique_key(type ## _Tab *tab, Arena *arena,					\
								type ## _Tab_Key hk, value v);							\
																						\
	MAC_HOT bool name ## _del(type ## _Tab *tab, key k);								\
																						\
	MAC_HOT bool name ## _del_key(type ## _Tab *tab, type ## _Tab_Key hk);				\
																						\
	void name ## _iter(type ## _Tab *tab, type ## _Tab_Iter_Fn fn, void *ctx);			\
																						\
	void name ## _iter_dump(type ## _Tab_Buck *buck, void *ctx);						\

TAB_DECL(str_tab, Str, Sized_Str, Sized_Str)

#endif
