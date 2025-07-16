#include "nml_const.h"
#include "nml_mac.h"
#include "nml_ast.h"
#include "nml_mem.h"
#include "nml_patn.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_fmt.h"

static const size_t sizes[] = {
#define NODE(kind, node, _) [PATN_ ## kind] = sizeof(Patn_ ## node),
#define PATN
#include "nml_node.h"
#undef PATN
#undef NODE
};

static MAC_INLINE inline void *
patn_node_new(Arena *arena, Patn_Kind kind)
{
	Patn_Node *node =  arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->type = TYPE_NONE;
	return (void *)node;
}

Patn_Node *
patn_ident_new(Arena *arena, Sized_Str name)
{
	Patn_Ident *node = patn_node_new(arena, PATN_IDENT);
	node->name = name;
	return (Patn_Node *)node;
}

Patn_Node *
patn_tuple_new(Arena *arena, Patn_Node **items, size_t len)
{
	Patn_Tuple *node = patn_node_new(arena, PATN_TUPLE);
	node->items = items;
	node->len = len;
	return (Patn_Node *)node;
}

Patn_Node *
patn_const_new(Arena *arena, Const_Value value)
{
	Patn_Const *node = patn_node_new(arena, PATN_CONST);
	node->value = value;
	return (Patn_Node *)node;
}

MAC_CONST bool
pattern_refutable(const Patn_Node *node)
{
	if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			if (pattern_refutable(tuple->items[i]))
			{
				return true;
			}
		}
		return false;
	}
	else if (node->kind == PATN_CONST)
	{
		return ((Patn_Const *)node)->value.kind != CONST_UNIT;
	}
	return false;
}

void
pattern_check(const Patn_Node *node, Arena *arena, Str_Set *set, Error_List *err)
{
	if (node->kind == PATN_IDENT)
	{
		Patn_Ident *ident = (Patn_Ident *)node;
		if (!STR_EQUAL(ident->name, STR_STATIC("_")))
		{
			if (str_set_unique(set, arena, ident->name) & SET_FAIL)
			{
				Sized_Str msg = format_str(arena,
											STR_STATIC("Identifier `{S}` used multiple times in pattern"),
											ident->name);
				error_append(err, arena, msg, 0, ERR_ERROR);
			}
		}
	}
	else if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			pattern_check(tuple->items[i], arena, set, err);
		}
	}
}
