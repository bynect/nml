#ifndef NML_SET_H
#define NML_SET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nml_mac.h"
#include "nml_hash.h"
#include "nml_str.h"

typedef enum {
	SET_DONE = 1 << 0,
	SET_FAIL = 1 << 1,
	SET_REALLOC = 1 << 2,
} Set_Result;

#define SET_DECL(name, type, key) 														\
	typedef struct {																	\
		key k;																			\
		Hash_Default hsh;																\
	} type ## _Set_Key;																	\
																						\
	typedef struct {																	\
		type ## _Set_Key hk;															\
		bool set;																		\
		bool tomb;																		\
	} type ## _Set_Buck;																\
																						\
	typedef struct {																	\
		type ## _Set_Buck *bucks;														\
		size_t len;																		\
		size_t size;																	\
		Hash_Default seed;																\
	} type ## _Set;																		\
																						\
	typedef void (*type ## _Set_Iter_Fn)(type ## _Set_Buck *buck, void *ctx);			\
																						\
	void name ## _init(type ## _Set *set, Hash_Default seed);							\
																						\
	void name ## _reset(type ## _Set *set);												\
																						\
	void name ## _free(type ## _Set *set);												\
																						\
	MAC_CONST type ## _Set_Key name ## _key(const type ## _Set *set, const key k);		\
																						\
	void name ## _union(const type ## _Set *MAC_RESTRICT set, type ## _Set *MAC_RESTRICT dest);	\
																						\
	void name ## _difference(const type ## _Set *set1, const type ## _Set *set2,		\
							type ## _Set *dest);										\
																						\
	void name ## _difference_key(const type ## _Set *set1, const type ## _Set *set2,	\
							type ## _Set *dest);										\
																						\
	MAC_HOT bool name ## _member(const type ## _Set *set, const key k);					\
																						\
	MAC_HOT bool name ## _member_key(const type ## _Set *set, type ## _Set_Key hk);		\
																						\
	MAC_HOT Set_Result name ## _insert(type ## _Set *set, key k);						\
									 													\
	MAC_HOT Set_Result name ## _insert_key(type ## _Set *set, type ## _Set_Key hk);		\
																						\
	MAC_HOT Set_Result name ## _unique(type ## _Set *set, key k);						\
																						\
	MAC_HOT Set_Result name ## _unique_key(type ## _Set *set, type ## _Set_Key hk);		\
																						\
	MAC_HOT bool name ## _remove(type ## _Set *set, key k);								\
																						\
	MAC_HOT bool name ## _remove_key(type ## _Set *set, type ## _Set_Key hk);			\
																						\
	void name ## _iter(type ## _Set *set, type ## _Set_Iter_Fn fn, void *ctx);			\
																						\
	void name ## _iter_dump(type ## _Set_Buck *buck, void *ctx);

SET_DECL(str_set, Str, Sized_Str)

#endif
