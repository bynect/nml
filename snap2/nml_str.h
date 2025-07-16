#ifndef NML_STR_H
#define NML_STR_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "nml_mem.h"
#include "nml_mac.h"

#define STR_STRLEN(str) 	(Sized_Str) { str, strlen(str) }
#define STR_STATIC(str) 	(Sized_Str) { MAC_STRSTATIC(str), MAC_STRLEN(str) }
#define STR_WLEN(str, len) 	(Sized_Str) { str, len }

#define STR_EQUAL(a, b)		((a).len == (b).len && !memcmp((a).str, (b).str, (a).len))

typedef struct Token Token;

typedef struct {
	const char *str;
	size_t len;
} Sized_Str;

MAC_CONST Sized_Str str_from_token(const Token *const tok);

MAC_CONST Sized_Str str_from_int(Arena *arena, const int64_t num, uint8_t base);

MAC_CONST int64_t str_to_int(const Sized_Str str, uint8_t base);

MAC_CONST double str_to_float(const Sized_Str str);

void str_append_int(Sized_Str *str, Arena *arena, const int64_t num, uint8_t base);

#endif
