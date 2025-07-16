#ifndef NML_CONST_H
#define NML_CONST_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_str.h"

typedef enum {
#define TAG(name, _, kind)	CONST_ ## name,
#define TYPE
#define BASE
#include "nml_tag.h"
#undef BASE
#undef TYPE
#undef TAG
} Const_Kind;

typedef struct {
	Const_Kind kind;
	union {
		int32_t c_int;
		double c_float;
		Sized_Str c_str;
		char c_char;
		bool c_bool;
	};
} Const_Value;

static inline void
const_init(Const_Value *c_value, Const_Kind kind, const void *value)
{
	c_value->kind = kind;
	switch (kind)
	{
		case CONST_UNIT:
			break;

		case CONST_INT:
			c_value->c_int = *(int32_t *)value;
			break;

		case CONST_FLOAT:
			c_value->c_float = *(double *)value;
			break;

		case CONST_STR:
			c_value->c_str = *(Sized_Str *)value;
			break;

		case CONST_CHAR:
			c_value->c_char = *(char *)value;
			break;

		case CONST_BOOL:
			c_value->c_bool = *(bool *)value;
			break;
	}
}

static inline Const_Value
const_new(Const_Kind kind, const void *value)
{
	Const_Value c_value;
	const_init(&c_value, kind, value);
	return c_value;
}

static inline bool
const_equal(Const_Value const1, Const_Value const2)
{
	if (const1.kind == const2.kind)
	{
		switch (const1.kind)
		{
			case CONST_UNIT:
				return true;

			case CONST_INT:
				return const1.c_int == const2.c_int;

			case CONST_FLOAT:
				return const1.c_float == const2.c_float;

			case CONST_STR:
				return STR_EQUAL(const1.c_str, const2.c_str);

			case CONST_CHAR:
				return const1.c_char == const2.c_char;

			case CONST_BOOL:
				return const1.c_bool == const2.c_bool;
		}
	}
	return false;
}

#endif
