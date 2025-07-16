#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "nml_fmt.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_type.h"
#include "nml_mem.h"
#include "nml_str.h"

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
format_append(Arena *arena, char **buf, size_t *size, size_t len)
{
	if (MAC_UNLIKELY(len >= *size))
	{
		*size *= 1.5;
		*buf = arena_update(arena, *buf, *size);
	}
}

static inline size_t
format_int(Arena *arena, char **buf, size_t *size, size_t len, uint64_t num, uint8_t base)
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

	format_append(arena, buf, size, len + i + 1);
	for (size_t j = 0; j < i; ++j)
	{
		(*buf)[len + j] = tmp[i - j - 1];
	}

	return len + i;
}

static inline size_t
format_tok_kind(Arena *arena, char **buf, size_t *size, size_t len, Token_Kind kind)
{
	Sized_Str str = toks[kind];
	format_append(arena, buf, size, len + str.len);
	memcpy(*buf + len, str.str, str.len);
	return len + str.len;
}

static inline size_t
format_tok(Arena *arena, char **buf, size_t *size, size_t len, Token tok)
{
	return format_tok_kind(arena, buf, size, len, tok.kind);
}

static inline size_t
format_type(Arena *arena, char **buf, size_t *size, size_t len, Type type,
			uint32_t fun_depth, uint32_t tuple_depth)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		const Type_Fun *fun = TYPE_PTR(type);
		if (fun_depth > 0)
		{
			format_append(arena, buf, size, len + 3);
			(*buf)[len++] = '(';
		}

		len = format_type(arena, buf, size, len, fun->par,
						fun_depth + 1, tuple_depth);

		while (TYPE_UNTAG(fun->ret) == TAG_FUN)
		{
			fun = TYPE_PTR(fun->ret);
			len = format_type(arena, buf, size, len, fun->par,
							fun_depth + 1, tuple_depth);

			format_append(arena, buf, size, len + 5);
			memcpy(*buf + len, " -> ", 4);
			len += 4;
		}

		format_append(arena, buf, size, len + 5);
		memcpy(*buf + len, " -> ", 4);
		len = format_type(arena, buf, size, len + 4, fun->ret,
							fun_depth + 1, tuple_depth);

		if (fun_depth > 0)
		{
			format_append(arena, buf, size, len + 3);
			(*buf)[len++] = ')';
		}
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		if (tuple_depth > 0)
		{
			format_append(arena, buf, size, len + 3);
			(*buf)[len++] = '(';
		}

		for (size_t i = 0; i < tuple->len - 1; ++i)
		{
			len = format_type(arena, buf, size, len, tuple->items[i],
								fun_depth + 1, tuple_depth + 1);

			format_append(arena, buf, size, len + 4);
			memcpy(*buf + len, " * ", 3);
			len += 3;
		}

		len = format_type(arena, buf, size, len, tuple->items[tuple->len - 1],
							fun_depth + 1, tuple_depth + 1);

		if (tuple_depth > 0)
		{
			format_append(arena, buf, size, len + 3);
			(*buf)[len++] = ')';
		}
	}
	else if (tag == TAG_VAR)
	{
		format_append(arena, buf, size, len + 2);
		(*buf)[len++] = 'V';
		return format_int(arena, buf, size, len, TYPE_TID(type), 10);
	}
	else if (tag == TAG_NAME)
	{
		Type_Name *name = TYPE_PTR(type);
		format_append(arena, buf, size, len + name->name.len);
		memcpy(*buf + len, name->name.str, name->name.len);
		len += name->name.len;
	}
	else if (tag == TAG_CONS)
	{
		Type_Cons *cons = TYPE_PTR(type);

		format_append(arena, buf, size, len + cons->name.len + 5);
		memcpy(*buf + len, "Var ", 4);
		len += 4;

		memcpy(*buf + len, cons->name.str, cons->name.len);
		len += cons->name.len;

		if (cons->cons != TYPE_NONE)
		{
			format_append(arena, buf, size, len + 2);
			(*buf)[len++] = ' ';
			(*buf)[len++] = '(';

			len = format_type(arena, buf, size, len, cons->cons,
								fun_depth, tuple_depth);

			format_append(arena, buf, size, len + 2);
			(*buf)[len++] = ')';
		}
	}
	else
	{
		Sized_Str str = types[tag];
		format_append(arena, buf, size, len + str.len);
		memcpy(*buf + len, str.str, str.len);
		len += str.len;
	}
	return len;
}

static inline size_t
format_sized_str(Arena *arena, char **buf, size_t *size, size_t len, Sized_Str str)
{
	format_append(arena, buf, size, len + str.len + 1);
	memcpy(*buf + len, str.str, str.len);
	return len + str.len;
}

Sized_Str
format_str(Arena *arena, Sized_Str fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	size_t size = fmt.len * 1.5;
	char *buf = arena_lock(arena, size);
	size_t len = 0;

	size_t i = 0;
	while (i < fmt.len)
	{
		if (MAC_UNLIKELY(fmt.str[i] == '{' &&
			i + 2 < fmt.len && fmt.str[i + 2] == '}'))
		{
			switch (fmt.str[i + 1])
			{
				case 'K':
					len = format_tok_kind(arena, &buf, &size, len, va_arg(args, Token_Kind));
					break;

				case 'T':
					len = format_tok(arena, &buf, &size, len, va_arg(args, Token));
					break;

				case 't':
					len = format_type(arena, &buf, &size, len, va_arg(args, Type), 0, 0);
					break;

				case 'i':
					len = format_int(arena, &buf, &size, len, va_arg(args, Type_Id), 10);
					break;

				case 'S':
					len = format_sized_str(arena, &buf, &size, len, va_arg(args, Sized_Str));
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

	buf = arena_update(arena, buf, len + 1);
	arena_unlock(arena, buf);
	buf[len] = '\0';

	va_end(args);
	return STR_WLEN(buf, len);
}
