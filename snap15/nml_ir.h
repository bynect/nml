#ifndef NML_IR_H
#define NML_IR_H

#include "nml_const.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"

typedef uint32_t Ir_Slot;

typedef enum {
	IR_VOID,
	IR_LABL,
	IR_PTR,
	IR_FUN,
	IR_TUPLE,
	IR_INT,
	IR_FLT,
} Type_Kind;

typedef struct Ir_Type {
	Type_Kind kind;
	union {
		const struct Ir_Type *ptr;
		struct {
			struct Ir_Type *ret;
			struct Ir_Type *args;
			size_t len;
		} fun;
		struct {
			struct Ir_Type *items;
			size_t len;
		} tuple;
		uint32_t width;
	};
} Ir_Type;

typedef enum {
	VAL_ERR,
	VAL_GLOB,
	VAL_LOCL,
	VAL_LABL,
	VAL_CONST,
} Value_Kind;

typedef struct {
	Value_Kind kind;
	Ir_Type type;
	union {
		Ir_Slot id;
		Sized_Str name;
		Const_Value value;
	};
} Ir_Value;

static MAC_INLINE inline Ir_Value
ir_error(void)
{
	return (Ir_Value) {
		.kind = VAL_ERR,
		.type = {
			.kind = IR_VOID,
		},
	};
}

static MAC_INLINE inline Ir_Value
ir_global(Ir_Type type, Sized_Str name)
{
	return (Ir_Value) {
		.kind = VAL_GLOB,
		.type = type,
		.name = name,
	};
}

static MAC_INLINE inline Ir_Value
ir_local(Ir_Type type, Ir_Slot id)
{
	return (Ir_Value) {
		.kind = VAL_LOCL,
		.type = type,
		.id = id,
	};
}

static MAC_INLINE inline Ir_Value
ir_label(Ir_Slot id)
{
	return (Ir_Value) {
		.kind = VAL_LABL,
		.type = {
			.kind = IR_LABL,
		},
		.id = id,
	};
}

static MAC_INLINE inline Ir_Value
ir_const(Const_Value value)
{
	return (Ir_Value) {
		.kind = VAL_CONST,
		.type = {
			.kind = IR_LABL,
		},
		.value = value,
	};
}

#endif
