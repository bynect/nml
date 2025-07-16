#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "nml_buf.h"
#include "nml_dump.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_check.h"

#define BUF_IMPL(name, type, value, valuecmp, valuedump,					\
				alloc, realloc, free, membase, memfactor)					\
																			\
	static MAC_INLINE inline void											\
	name ## _grow(type ## _Buf *buf, size_t size)							\
	{																		\
		if (MAC_UNLIKELY(size > buf->size))									\
		{																	\
			buf->size *= memfactor;											\
			buf->buf = realloc(buf->buf, buf->size * sizeof(value));		\
		}																	\
	}																		\
																			\
	void																	\
	name ## _init(type ## _Buf *buf)										\
	{																		\
		buf->buf = alloc(membase * sizeof(value));							\
		buf->len = 0;														\
		buf->size = membase;												\
	}																		\
																			\
	void																	\
	name ## _reset(type ## _Buf *buf)										\
	{																		\
		buf->len = 0;														\
	}																		\
																			\
	void																	\
	name ## _free(type ## _Buf *buf)										\
	{																		\
		free(buf->buf);														\
	}																		\
																			\
	void																	\
	name ## _union(const type ## _Buf *src, type ## _Buf *dest)				\
	{																		\
		name ## _grow(dest, dest->len + src->len);							\
		size_t size = src->len * sizeof(value);								\
		memcpy(dest->buf + dest->len, src->buf, size);						\
		dest->len += src->len;												\
	}																		\
																			\
	MAC_HOT void															\
	name ## _push(type ## _Buf *buf, const value v)							\
	{																		\
		name ## _grow(buf, buf->len + 1);									\
		buf->buf[buf->len++] = v;											\
	}																		\
																			\
	MAC_HOT void															\
	name ## _pushs(type ## _Buf *buf, const value *v, size_t len)			\
	{																		\
		name ## _grow(buf, buf->len + len);									\
		memcpy(buf->buf + buf->len, v, len * sizeof(value));				\
	}																		\
																			\
	MAC_HOT	bool															\
	name ## _pop(type ## _Buf *buf)											\
	{																		\
		if (MAC_UNLIKELY(buf->len == 0))									\
		{																	\
			return false;													\
		}																	\
																			\
		--buf->len;															\
		return true;														\
	}																		\
																			\
	MAC_HOT	size_t															\
	name ## _pops(type ## _Buf *buf, size_t len)							\
	{																		\
		len = buf->len > len ? buf->len - len : buf->len;					\
		buf->len -= len;													\
		return len;															\
	}																		\
																			\
	MAC_HOT bool															\
	name ## _contain(type ## _Buf *buf, const value v)						\
	{																		\
		MAC_PREFETCH_CACHE(buf->buf, 1);									\
		for (size_t i = 0; i < buf->len; ++i)								\
		{																	\
			if (valuecmp(buf->buf[i], v))									\
			{																\
				return true;												\
			}																\
		}																	\
		return false;														\
	}																		\
																			\
	void																	\
	name ## _iter(type ## _Buf *buf, type ## _Buf_Iter_Fn fn, void *ctx)	\
	{																		\
		for (size_t i = 0; i < buf->len; ++i)								\
		{																	\
			fn(buf->buf + i, ctx);											\
		}																	\
	}																		\
																			\
	void																	\
	name ## _iter_dump(value *v, void *ctx)									\
	{																		\
		MAC_UNUSED(ctx);													\
																			\
		fprintf(stdout, "Value dump: ");									\
		valuedump(v);														\
		fprintf(stdout, "\n");												\
	}																		\

static MAC_CONST bool
constr_buf_equal(Type t1, Type t2)
{
	const Type_Tag tag1 = TYPE_UNTAG(t1);
	const Type_Tag tag2 = TYPE_UNTAG(t2);

	if (tag1 == TAG_FUN && tag2 == TAG_FUN)
	{
		Type_Fun *fun1 = TYPE_PTR(t1);
		Type_Fun *fun2 = TYPE_PTR(t2);

		return constr_buf_equal(fun1->par, fun2->par) &&
				constr_buf_equal(fun1->ret, fun2->ret);
	}
	else if (tag1 == TAG_TUPLE && tag2 == TAG_TUPLE)
	{
		Type_Tuple *tuple1 = TYPE_PTR(t1);
		Type_Tuple *tuple2 = TYPE_PTR(t2);

		if (tuple1->len != tuple2->len)
		{
			return false;
		}

		for (size_t i = 0; i < tuple1->len; ++i)
		{
			if (!constr_buf_equal(tuple1->items[i], tuple2->items[i]))
			{
				return false;
			}
		}
		return true;
	}
	else if (tag1 == TAG_VAR && tag2 == TAG_VAR)
	{
		return TYPE_TID(t1) == TYPE_TID(t2);
	}

	return tag1 == tag2;
}

#define BUF_EQ(a, b)					((a) == (b))

#define BUF_CONSTREQ(a, b)			(constr_buf_equal(a.t1, b.t1) && constr_buf_equal(a.t2, b.t2))
#define BUF_CONSTRDUMP(value)			\
	do 									\
	{									\
		fprintf(stdout, "\nt1: ");		\
		dump_type(value->t1);		\
										\
		fprintf(stdout, "t2: ");		\
		dump_type(value->t2);		\
	}									\
	while (false)

#define BUF_BYTEDUMP(value)				fprintf(stdout, "%hxx", *value)

BUF_IMPL(constr_buf, Constr, Type_Constr, BUF_CONSTREQ, BUF_CONSTRDUMP,
		malloc, realloc, free, 16, 2)

//BUF_DECL(byte_buf, Byte, uint8_t)
//BUF_IMPL(byte_buf, Byte, uint8_t, BUF_EQ, BUF_BYTEDUMP,
//		malloc, realloc, free, 16, 2)
