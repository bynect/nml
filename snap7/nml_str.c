#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_lex.h"

MAC_CONST Sized_Str
str_from_token(const Token *const tok)
{
	if (tok->kind == TOK_STR || tok->kind == TOK_CHAR)
	{
		return (Sized_Str) {
			.str = tok->str.str + 1,
			.len = tok->str.len - 1,
		};
	}
	else
	{
		return tok->str;
	}
}

static MAC_CONST size_t
str_from_int_len(char *tmp, int64_t num, uint8_t base)
{
	int64_t acc = num;
	size_t i = 0;

	do
	{
		int64_t n = acc % base;
		tmp[i++] = (n < 10 ? '0' : 'a' - 10) + n;
		acc /= base;
	}
	while (acc != 0);
	return i;
}

MAC_CONST Sized_Str
str_from_int(Arena *arena, const int64_t num, uint8_t base)
{
	char tmp[128];
	size_t len = str_from_int_len(tmp, num, base);

	char *str = arena_alloc(arena, len + 1);
	str[len] = '\0';

	for (size_t j = 0; j < len; ++j)
	{
		str[j] = tmp[len - j - 1];
	}
	return STR_WLEN(str, len);
}

MAC_CONST int64_t
str_to_int(const Sized_Str str, uint8_t base)
{
	int64_t acc = 0;
	for (size_t i = 0; i < str.len; ++i)
	{
		char c = str.str[i];

		if (MAC_LIKELY(c >=  '0' && c <= '9'))
		{
			c -= '0';
		}
		else if (c >= 'a' && c <= 'f')
		{
			c -= 'a' - 10;
		}
		else if (c >= 'A' && c <= 'F')
		{
			c -= 'A' - 10;
		}

		acc = acc * base + c;
	}
	return acc;
}

MAC_CONST double
str_to_float(const Sized_Str str)
{
	double acc = 0;
	int32_t e = 0;
	size_t i = 0;

	while (i < str.len && isdigit(str.str[i]))
	{
		acc = acc * 10.0 + (str.str[i++] - '0');
	}

	if (str.str[i] == '.' && ++i)
	{
		while (i < str.len && isdigit(str.str[i]))
		{
			acc = (str.str[i++] - '0') + acc * 10.0;
			e -= 1;
		}
	}

	if ((str.str[i] == 'e' || str.str[i] == 'E') && ++i)
	{
		bool neg = false;
		int32_t exp = 0;

		if (str.str[i] == '-')
		{
			neg = true;
			++i;
		}
		else if (str.str[i] == '+')
		{
			++i;
		}

		while (i < str.len && isdigit(str.str[i]))
		{
			exp = (str.str[i++] - '0') + exp * 10;
		}
		e += neg ? -exp : exp;
	}

	while (e > 0)
	{
		acc *= 10;
		--e;
	}

	while (e < 0)
	{
		acc *= 0.1;
		++e;
	}

	return acc;
}

void
str_append_int(Sized_Str *str, Arena *arena, const int64_t num, uint8_t base)
{
	char tmp[64];
	size_t len = str_from_int_len(tmp, num, base) + str->len;

	char *buf = arena_alloc(arena, len + 1);
	memcpy(buf, str->str, str->len);
	buf[len] = '\0';

	for (size_t j = str->len; j < len; ++j)
	{
		buf[j] = tmp[len - j - 1];
	}

	str->str = buf;
	str->len = len;
}

