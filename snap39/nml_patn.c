#include "nml_const.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_patn.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_fmt.h"
#include "nml_expr.h"
#include "nml_type.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const size_t sizes[] = {
#define NODE(kind, node, _) [PATN_ ## kind] = sizeof(Patn_ ## node),
#define PATN
#include "nml_node.h"
#undef PATN
#undef NODE
};

static MAC_INLINE inline void *
patn_node_new(Arena *arena, Location loc, Patn_Kind kind)
{
	Patn_Node *node = arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->type = TYPE_NONE;
	node->loc = loc;
	return (void *)node;
}

Patn_Node *
patn_ident_new(Arena *arena, Location loc, Sized_Str name)
{
	Patn_Ident *node = patn_node_new(arena, loc, PATN_IDENT);
	node->name = name;
	return (Patn_Node *)node;
}

Patn_Node *
patn_tuple_new(Arena *arena, Location loc, Patn_Node **items, size_t len)
{
	Patn_Tuple *node = patn_node_new(arena, loc, PATN_TUPLE);
	node->items = items;
	node->len = len;
	return (Patn_Node *)node;
}

Patn_Node *
patn_const_new(Arena *arena, Location loc, Const_Value value)
{
	Patn_Const *node = patn_node_new(arena, loc, PATN_CONST);
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
		return MAC_CAST(Patn_Const *, node)->value.kind != CONST_UNIT;
	}
	return false;
}

MAC_HOT bool
pattern_check(const Patn_Node *node, Arena *arena, Str_Set *set, Error_List *err)
{
	bool ret = true;
	if (node->kind == PATN_IDENT)
	{
		Patn_Ident *ident = (Patn_Ident *)node;
		if (!STR_EQUAL(ident->name, STR_STATIC("_")))
		{
			if (str_set_unique(set, ident->name) & SET_FAIL)
			{
				Sized_Str msg = format_str(arena,
											STR_STATIC("Identifier `{S}` used multiple times in pattern"),
											ident->name);
				error_append(err, arena, msg, ident->node.loc, ERR_ERROR);
				ret = false;
			}
		}
	}
	else if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			if (!pattern_check(tuple->items[i], arena, set, err))
			{
				ret = false;
			}
		}
	}
	return ret;
}

MAC_HOT bool
pattern_match_check(const Expr_Match *node, Arena *arena, Hash_Default seed, Error_List *err)
{
	bool succ = true;
	bool wildcard = false;

	Str_Set set;
	str_set_init(&set, seed);

	for (size_t i = 0; i < node->len; ++i)
	{
		wildcard = wildcard || !pattern_refutable(node->arms[i].patn);
		succ =  pattern_check(node->arms[i].patn, arena, &set, err) && succ;
		str_set_reset(&set);
	}
	str_set_free(&set);

	if (succ && !wildcard)
	{
		bool exhaust = true;
		if (node->value->type == TYPE_CHAR)
		{
			bool cases[UINT8_MAX] = {false};
			for (size_t i = 0; i < node->len; ++i)
			{
				if (node->arms[i].patn->kind == PATN_CONST)
				{
					cases[(uint8_t)MAC_CAST(Patn_Const *, node->arms[i].patn)->value.c_char] = true;
				}
			}

			for (size_t i = 0; i < UINT8_MAX; ++i)
			{
				exhaust = exhaust && cases[i];
			}
		}
		else if (node->value->type == TYPE_BOOL)
		{
			bool cases[2] = {false, false};
			for (size_t i = 0; i < node->len; ++i)
			{
				if (node->arms[i].patn->kind == PATN_CONST)
				{
					cases[MAC_CAST(Patn_Const *, node->arms[i].patn)->value.c_bool] = true;
				}
			}
			exhaust = cases[0] && cases[1];
		}
		else if (node->value->type != TYPE_UNIT)
		{
			exhaust = false;
		}

		if (!exhaust)
		{
			error_append(err, arena, STR_STATIC("Non exhaustive match"), node->node.loc, ERR_ERROR);
			succ = false;
		}
	}
	return succ;
}
