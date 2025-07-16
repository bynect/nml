#include "nml_box.h"
#include "nml_str.h"

static const size_t sizes[] = {
#define TAG(name, id, _)	[BOX_ ## name] = sizeof(Box_ ## id),
#define BOX
#include "nml_tag.h"
#undef BOX
#undef TAG
};

static MAC_INLINE inline void *
box_new(Arena *arena, Box_Kind kind)
{
	Box *box =  arena_alloc(arena, sizes[kind]);
	box->kind = kind;
	return (void *)box;
}

Box *
box_fun_new(Arena *arena)
{
	Box_Fun *box = box_new(arena, BOX_FUN);
	return (Box *)box;
}

Box *
box_tuple_new(Arena *arena, Value *items, size_t len)
{
	Box_Tuple *box = box_new(arena, BOX_TUPLE);
	box->items = items;
	box->len = len;
	return (Box *)box;
}

Box *
box_str_new(Arena *arena, Sized_Str str)
{
	Box_Str *box = box_new(arena, BOX_STR);
	box->str = str;
	return (Box *)box;
}
