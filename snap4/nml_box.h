#ifndef NML_BOX_H
#define NML_BOX_H

#include "nml_str.h"
#include "nml_mac.h"
#include "nml_bc.h"
#include "nml_val.h"
#include "nml_mem.h"

typedef enum {
#define TAG(tag, _, kind)	BOX_ ## tag,
#define BOX
#include "nml_tag.h"
#undef BOX
#undef TAG
} Box_Kind;

#define VAL_ISFUN(val)		(value_is_box(val, BOX_FUN))
#define VAL_ISTUPLE(val)	(value_is_box(val, BOX_TUPLE))
#define VAL_ISSTR(val)		(value_is_box(val, BOX_STR))

typedef enum {
	FUN_BYTECODE,
	FUN_NATIVE,
} Fun_Kind;

typedef struct Box {
	Box_Kind kind;
} Box;

typedef struct {
	Box box;
	Sized_Str name;
	Fun_Kind kind;
	union {
		const uint8_t *bc;
		Value (*ptr)(const Value *stack);
	} fun;
} Box_Fun;

typedef struct {
	Box box;
	Value *items;
	size_t len;
} Box_Tuple;

typedef struct {
	Box box;
	Sized_Str str;
} Box_Str;

Box *box_fun_new(Arena *arena);

Box *box_tuple_new(Arena *arena, Value *items, size_t len);

Box *box_str_new(Arena *arena, Sized_Str str);

static MAC_INLINE inline bool
value_is_box(Value val, Box_Kind kind)
{
	return VAL_ISBOX(val) && VAL_UNBOX(val)->kind == kind;
}

#endif
