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

#define PATN_NEW(_node, _kind, _loc)					\
	arena_alloc(arena, sizeof(_node));					\
	MAC_CAST(Patn_Node *, node)->kind = _kind;			\
	MAC_CAST(Patn_Node *, node)->type = TYPE_NONE;		\
	MAC_CAST(Patn_Node *, node)->loc = _loc

Patn_Node *
patn_ident_new(Arena *arena, Location loc, Sized_Str name)
{
	Patn_Ident *node = PATN_NEW(Patn_Ident, PATN_IDENT, loc);
	node->name = name;
	return (Patn_Node *)node;
}

Patn_Node *
patn_constr_new(Arena *arena, Location loc, Sized_Str name, Patn_Node *patn)
{
	Patn_Constr *node = PATN_NEW(Patn_Constr, PATN_CONSTR, loc);
	node->name = name;
	node->patn = patn;
	return (Patn_Node *)node;
}

Patn_Node *
patn_as_new(Arena *arena, Location loc, Sized_Str name, Patn_Node *patn)
{
	Patn_As *node = PATN_NEW(Patn_As, PATN_AS, loc);
	node->name = name;
	node->patn = patn;
	return (Patn_Node *)node;
}

Patn_Node *
patn_tuple_new(Arena *arena, Location loc, Patn_Node **items, size_t len)
{
	Patn_Tuple *node = PATN_NEW(Patn_Tuple, PATN_TUPLE, loc);
	node->items = items;
	node->len = len;
	return (Patn_Node *)node;
}

Patn_Node *
patn_const_new(Arena *arena, Location loc, Const_Value value)
{
	Patn_Const *node = PATN_NEW(Patn_Const, PATN_CONST, loc);
	node->value = value;
	return (Patn_Node *)node;
}

MAC_CONST bool
patn_refutable(const Patn_Node *node)
{
	if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			if (patn_refutable(tuple->items[i]))
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
patn_check(const Patn_Node *node, Arena *arena, Str_Set *set, Error_List *err)
{
	bool ret = true;
	switch (node->kind)
	{
		case PATN_IDENT:
			if (!STR_EQUAL(MAC_CAST(Patn_Ident *, node)->name, STR_STATIC("_")))
			{
				if (str_set_unique(set, MAC_CAST(Patn_Ident *, node)->name) & SET_FAIL)
				{
					Sized_Str msg = format_str(arena,
												STR_STATIC("Identifier `{S}` used multiple times in pattern"),
												MAC_CAST(Patn_Ident *, node)->name);
					error_append(err, arena, msg, node->loc, ERR_ERROR);
					ret = false;
				}
			}
			break;

		case PATN_AS:
			if (!patn_check(MAC_CAST(Patn_As *, node)->patn, arena, set, err))
			{
				ret = false;
			}

			if (str_set_unique(set, MAC_CAST(Patn_As *, node)->name) & SET_FAIL)
			{
				Sized_Str msg = format_str(arena,
											STR_STATIC("Identifier `{S}` used multiple times in pattern"),
											MAC_CAST(Patn_As *, node)->name);
				error_append(err, arena, msg, node->loc, ERR_ERROR);
				ret = false;
			}
			break;

		case PATN_TUPLE:
			for (size_t i = 0; i < MAC_CAST(Patn_Tuple *, node)->len; ++i)
			{
				if (!patn_check(MAC_CAST(Patn_Tuple *, node)->items[i], arena, set, err))
				{
					ret = false;
				}
			}
			break;

		default:
			break;

	}
	return ret;
}

MAC_HOT bool
patn_match_check(const Expr_Match *node, Arena *arena, Str_Set *set, Error_List *err)
{
	bool succ = true;
	bool wildcard = false;

	for (size_t i = 0; i < node->len; ++i)
	{
		str_set_reset(set);
		wildcard = wildcard || !patn_refutable(node->arms[i].patn);
		succ =  patn_check(node->arms[i].patn, arena, set, err) && succ;
	}

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
