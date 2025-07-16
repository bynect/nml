#ifndef NML_LIT_H
#define NML_LIT_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_str.h"

typedef enum {
#define TAG(name, _, kind)	LIT_ ## name,
#define TYPE
#define PRIM
#include "nml_tag.h"
#undef PRIM
#undef TYPE
#undef TAG
} Lit_Kind;

typedef struct {
	Lit_Kind kind;
	union {
		int32_t v_int;
		double v_float;
		Sized_Str v_str;
		char v_char;
		bool v_bool;
	};
} Lit_Value;

static inline void
const_init(Lit_Value *lit, Lit_Kind kind, const void *value)
{
	lit->kind = kind;
	switch (kind)
	{
		case LIT_UNIT:
			break;

		case LIT_INT:
			lit->v_int = *(int32_t *)value;
			break;

		case LIT_FLOAT:
			lit->v_float = *(double *)value;
			break;

		case LIT_STR:
			lit->v_str = *(Sized_Str *)value;
			break;

		case LIT_CHAR:
			lit->v_char = *(char *)value;
			break;

		case LIT_BOOL:
			lit->v_bool = *(bool *)value;
			break;
	}
}

static MAC_CONST inline Lit_Value
const_new(Lit_Kind kind, const void *value)
{
	Lit_Value v_value;
	const_init(&v_value, kind, value);
	return v_value;
}

static MAC_CONST inline bool
const_equal(Lit_Value lit1, Lit_Value lit2)
{
	if (lit1.kind == lit2.kind)
	{
		switch (lit1.kind)
		{
			case LIT_UNIT:
				return true;

			case LIT_INT:
				return lit1.v_int == lit2.v_int;

			case LIT_FLOAT:
				return lit1.v_float == lit2.v_float;

			case LIT_STR:
				return STR_EQUAL(lit1.v_str, lit2.v_str);

			case LIT_CHAR:
				return lit1.v_char == lit2.v_char;

			case LIT_BOOL:
				return lit1.v_bool == lit2.v_bool;
		}
	}
	return false;
}

#endif
