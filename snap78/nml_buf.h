#ifndef NML_BUF_H
#define NML_BUF_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "nml_arena.h"
#include "nml_str.h"

#define BUF_DECL(name, type, value)												\
	typedef struct {															\
		value *buf;																\
		size_t len;																\
		size_t size;															\
	} type ## _Buf;																\
																				\
	typedef void (*type ## _Buf_Iter_Fn)(value *v, void *ctx);					\
																				\
	void name ## _init(type ## _Buf *buf);										\
																				\
	void name ## _reset(type ## _Buf *buf);										\
																				\
	void name ## _free(type ## _Buf *buf);										\
																				\
	void name ## _reserve(type ## _Buf *buf, size_t len);						\
																				\
	void name ## _append(const type ## _Buf *src, type ## _Buf *dest);			\
																				\
	MAC_HOT bool name ## _member(const type ## _Buf *buf, const value v);		\
																				\
	MAC_HOT void name ## _push(type ## _Buf *buf, const value v);				\
																				\
	MAC_HOT void name ## _pushs(type ## _Buf *buf, const value *v, size_t len);	\
																				\
	MAC_HOT bool name ## _pop(type ## _Buf *buf);								\
																				\
	MAC_HOT size_t name ## _pops(type ## _Buf *buf, size_t len);				\
																				\
	MAC_HOT void name ## _unique(type ## _Buf *buf, const value v);				\
																				\
	void name ## _iter(type ## _Buf *buf, type ## _Buf_Iter_Fn fn, void *ctx);	\
																				\
	void name ## _iter_dump(value *v, void *ctx);

BUF_DECL(str_buf, Str, Sized_Str)

BUF_DECL(byte_buf, Byte, uint8_t)

#endif
