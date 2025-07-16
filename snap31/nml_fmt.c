#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "nml_fmt.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"

#define FMT_STATIC	1024

static const Sized_Str toks[] = {
#define TOK(type, str, id, kind) [TOK_ ## type] = STR_STATIC(kind(str, id)),
#define SPEC(str, _) str
#define EXTRA(str, _) str
#define DELIM(str, _) str
#define OPER(str, _) str
#define LIT(str, _) str
#define KWD(_, id) #id
#include "nml_tok.h"
#undef SPEC
#undef EXTRA
#undef DELIM
#undef OPER
#undef LIT
#undef KWD
#undef TOK
};

static const Sized_Str types[] = {
#define TAG(tag, id, _)	[TAG_ ## tag] = STR_STATIC(#id),
#define TYPE
#include "nml_tag.h"
#undef TYPE
#undef TAG
};

static inline void
format_append(char **buf, size_t *size, size_t len)
{
	if (MAC_UNLIKELY(len >= *size))
	{
		size_t memsize = *size * 1.5;
		if (MAC_UNLIKELY(*size < FMT_STATIC))
		{
			memsize = memsize > FMT_STATIC ? memsize : FMT_STATIC * 1.5;
			char *tmp = malloc(memsize);
			memcpy(tmp, *buf, *size);
			*buf = tmp;
		}
		else
		{
			*buf = realloc(*buf, memsize);
		}
		*size = memsize;
	}
}

static inline size_t
format_int64(char **buf, size_t *size, size_t len, int64_t num, uint8_t base)
{
	char tmp[64];
	int64_t acc = num;
	size_t i = 0;

	do
	{
		int64_t n = acc % base;
		tmp[i++] = (n < 10 ? '0' : 'a' - 10) + n;
		acc /= base;
	}
	while (acc != 0);

	format_append(buf, size, len + i + 3);
	if (num < 0)
	{
		(*buf)[len++] = '-';
	}

	for (size_t j = 0; j < i; ++j)
	{
		(*buf)[len + j] = tmp[i - j - 1];
	}

	return len + i;
}

static MAC_INLINE inline size_t
format_int32(char **buf, size_t *size, size_t len, int32_t num, uint8_t base)
{
	return format_int64(buf, size, len, (int64_t)num, base);
}

static inline size_t
format_uint64(char **buf, size_t *size, size_t len, uint64_t num, uint8_t base)
{
	char tmp[64];
	int64_t acc = num;
	size_t i = 0;

	do
	{
		int64_t n = acc % base;
		tmp[i++] = (n < 10 ? '0' : 'a' - 10) + n;
		acc /= base;
	}
	while (acc != 0);

	format_append(buf, size, len + i + 2);
	for (size_t j = 0; j < i; ++j)
	{
		(*buf)[len + j] = tmp[i - j - 1];
	}

	return len + i;
}

static MAC_INLINE inline size_t
format_uint32(char **buf, size_t *size, size_t len, uint32_t num, uint8_t base)
{
	return format_uint64(buf, size, len, (uint64_t)num, base);
}

static MAC_INLINE inline size_t
format_sized_str(char **buf, size_t *size, size_t len, Sized_Str str)
{
	format_append(buf, size, len + str.len + 1);
	memcpy(*buf + len, str.str, str.len);
	return len + str.len;
}

static inline size_t
format_tok_kind(char **buf, size_t *size, size_t len, Token_Kind kind)
{
	Sized_Str str = toks[kind];
	format_append(buf, size, len + str.len);
	memcpy(*buf + len, str.str, str.len);
	return len + str.len;
}

static inline size_t
format_tok(char **buf, size_t *size, size_t len, Token tok)
{
	len = format_tok_kind(buf, size, len, tok.kind);
	if (tok.kind >= TOK_IDENT && tok.kind <= TOK_CHAR)
	{
		format_append(buf, size, len + tok.str.len + 3);
		(*buf)[len++] = '(';
		memcpy(*buf + len, tok.str.str, tok.str.len);
		len += tok.str.len;
		(*buf)[len++] = ')';
	}
	return len;
}

static inline size_t
format_type(char **buf, size_t *size, size_t len, Type type, uint32_t fun_depth, uint32_t tuple_depth)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		const Type_Fun *fun = TYPE_PTR(type);

		if (fun_depth > 0)
		{
			format_append(buf, size, len + 3);
			(*buf)[len++] = '(';
		}
		len = format_type(buf, size, len, fun->par,	fun_depth + 1, tuple_depth);

		while (TYPE_UNTAG(fun->ret) == TAG_FUN)
		{
			format_append(buf, size, len + 5);
			memcpy(*buf + len, " -> ", 4);
			len += 4;

			fun = TYPE_PTR(fun->ret);
			len = format_type(buf, size, len, fun->par, fun_depth + 1, tuple_depth);
		}

		format_append(buf, size, len + 5);
		memcpy(*buf + len, " -> ", 4);
		len = format_type(buf, size, len + 4, fun->ret, fun_depth + 1, tuple_depth);

		if (fun_depth > 0)
		{
			format_append(buf, size, len + 3);
			(*buf)[len++] = ')';
		}
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		if (tuple_depth > 0)
		{
			format_append(buf, size, len + 3);
			(*buf)[len++] = '(';
		}

		for (size_t i = 0; i < tuple->len - 1; ++i)
		{
			len = format_type(buf, size, len, tuple->items[i], fun_depth + 1, tuple_depth + 1);
			format_append(buf, size, len + 4);
			memcpy(*buf + len, " * ", 3);
			len += 3;
		}

		len = format_type(buf, size, len, tuple->items[tuple->len - 1],	fun_depth + 1, tuple_depth + 1);
		if (tuple_depth > 0)
		{
			format_append(buf, size, len + 2);
			(*buf)[len++] = ')';
		}
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		return format_sized_str(buf, size, len, var->var);
	}
	else
	{
		Sized_Str name = types[tag];
		return format_sized_str(buf, size, len, name);
	}
	return len;
}

static inline size_t
format_type_scheme(char **buf, size_t *size, size_t len, Type_Scheme scheme)
{
	if (scheme.len != 0)
	{
		format_append(buf, size, len + 8);
		memcpy(*buf + len, "forall", 6);
		len += 6;

		for (size_t i = 0; i < scheme.len; ++i)
		{
			(*buf)[len++] = ' ';
			len = format_sized_str(buf, size, len, scheme.vars[i]);
		}

		format_append(buf, size, len + 3);
		(*buf)[len++] = '.';
		(*buf)[len++] = ' ';
	}
	return format_type(buf, size, len, scheme.type, 0, 0);
}

Sized_Str
format_str(Arena *arena, Sized_Str fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	size_t size = fmt.len * 1.5;
	size_t len = 0;

	char stat[FMT_STATIC];
	char *buf = size < FMT_STATIC ? stat : malloc(size);

	size_t i = 0;
	while (i < fmt.len)
	{
		if (MAC_UNLIKELY(fmt.str[i] == '{' &&
			i + 2 < fmt.len && fmt.str[i + 2] == '}'))
		{
			switch (fmt.str[i + 1])
			{
				case 'i':
					len = format_int32(&buf, &size, len, va_arg(args, int32_t), 10);
					break;

				case 'I':
					len = format_int64(&buf, &size, len, va_arg(args, int64_t), 10);
					break;

				case 'u':
					len = format_uint32(&buf, &size, len, va_arg(args, uint32_t), 10);
					break;

				case 'U':
					len = format_uint64(&buf, &size, len, va_arg(args, uint64_t), 10);
					break;

				case 'x':
					len = format_uint32(&buf, &size, len, va_arg(args, uint32_t), 16);
					break;

				case 'X':
					len = format_uint64(&buf, &size, len, va_arg(args, uint64_t), 16);
					break;

				case 'S':
					len = format_sized_str(&buf, &size, len, va_arg(args, Sized_Str));
					break;

				case 'K':
					len = format_tok_kind(&buf, &size, len, va_arg(args, Token_Kind));
					break;

				case 'T':
					len = format_tok(&buf, &size, len, va_arg(args, Token));
					break;

				case 't':
					len = format_type(&buf, &size, len, va_arg(args, Type), 0, 0);
					break;

				case 's':
					len = format_type_scheme(&buf, &size, len, va_arg(args, Type_Scheme));
					break;

				default:
					break;
			}
			i += 3;
		}
		else
		{
			buf[len++] = fmt.str[i++];
		}
	}

	char *copy = arena_alloc(arena, len + 1);
	memcpy(copy, buf, len);
	copy[len] = '\0';

	if (size > FMT_STATIC)
	{
		free(buf);
	}

	va_end(args);
	return STR_WLEN(copy, len);
}
