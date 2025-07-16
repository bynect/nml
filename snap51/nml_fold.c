#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "nml_ast.h"
#include "nml_const.h"
#include "nml_decl.h"
#include "nml_err.h"
#include "nml_expr.h"
#include "nml_fold.h"
#include "nml_hash.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_patn.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_type.h"

static MAC_HOT void folder_expr(Folder *fold, Expr_Node **node, Expr_Tab *tab);

static MAC_INLINE inline void
folder_warn(Folder *fold, Location loc, Sized_Str msg)
{
	error_append(fold->err, fold->arena, msg, loc, ERR_WARN);
}

static MAC_INLINE inline void
folder_type(Folder *fold, Expr_Node *node, Type type)
{
	if (MAC_UNLIKELY(node->type != TYPE_NONE && node->type != type))
	{
		folder_warn(fold, node->loc, STR_STATIC("Mismatched types"));
	}
	node->type = type;
}

static MAC_CONST inline bool
folder_const(Expr_Node *node)
{
	return node->kind == EXPR_CONST;
}

static MAC_INLINE inline Expr_Node *
folder_const_copy(Folder *fold, Expr_Const *node)
{
	Expr_Const *tmp = arena_alloc(fold->arena, sizeof(Expr_Const));
	memcpy(tmp, node, sizeof(Expr_Const));
	return (Expr_Node *)tmp;
}

static MAC_HOT void
folder_if(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_If *cast = (Expr_If *)*node;
	folder_expr(fold, &cast->cond, tab);

	if (folder_const(cast->cond))
	{
		if (MAC_CAST(Expr_Const *, cast->cond)->value.c_bool)
		{
			*node = cast->b_then;
			folder_warn(fold, cast->cond->loc, STR_STATIC("Condition is always true"));
		}
		else
		{
			*node = cast->b_else;
			folder_warn(fold, cast->cond->loc, STR_STATIC("Condition is always false"));
		}

		folder_expr(fold, node, tab);
		return;
	}

	folder_expr(fold, &cast->b_then, tab);
	folder_expr(fold, &cast->b_else, tab);

	if (folder_const(cast->b_then) && folder_const(cast->b_else))
	{
		if (const_equal(MAC_CAST(Expr_Const *, cast->b_then)->value,
						MAC_CAST(Expr_Const *, cast->b_else)->value))
		{
			*node = cast->b_then;
		}
	}
}

static MAC_HOT void
folder_match(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Match *cast = (Expr_Match *)*node;
	folder_expr(fold, &cast->value, tab);

	if (MAC_UNLIKELY(cast->arms[0].patn->kind == PATN_IDENT))
	{
		// NOTE: If the first pattern is a wildcard directly substitute it
		if (folder_const(cast->value) || cast->value->kind == EXPR_IDENT)
		{
			Patn_Ident *ident = (Patn_Ident *)cast->arms[0].patn;
			if (!STR_EQUAL(ident->name, STR_STATIC("_")))
			{
				Expr_Node *value;
				bool shad = expr_tab_get(tab, ident->name, &value);
				expr_tab_insert(tab, ident->name, cast->value);

				folder_expr(fold, &cast->arms[0].expr, tab);
				if (shad)
				{
					expr_tab_insert(tab, ident->name, value);
				}
				else
				{
					expr_tab_remove(tab, ident->name);
				}
			}
			else
			{
				folder_warn(fold, cast->arms[0].patn->loc,
							STR_STATIC("First pattern in match is a wildcard"));
			}
			*node = cast->arms[0].expr;
		}
		else
		{
			// TODO: Check for side effects
			*node = expr_let_new(fold->arena, cast->node.loc, TYPE_NONE,
								MAC_CAST(Patn_Ident *, cast->arms[0].patn)->name,
								cast->value, cast->arms[0].expr);

			folder_type(fold, *node, cast->arms[0].expr->type);
			folder_expr(fold, node, tab);
		}
		return;
	}
	else if (MAC_UNLIKELY(cast->arms[0].patn->kind == PATN_CONST &&
			MAC_CAST(Patn_Const *, cast->arms[0].patn)->value.kind == CONST_UNIT))
	{
		*node = cast->arms[0].expr;
		folder_expr(fold, node, tab);
		return;
	}
	else if (folder_const(cast->value))
	{
		for (size_t i = 0; i < cast->len; ++i)
		{
			if (cast->arms[i].patn->kind == PATN_CONST &&
				const_equal(MAC_CAST(Expr_Const *, cast->value)->value, MAC_CAST(Patn_Const *, cast->arms[i].patn)->value))
			{
				folder_expr(fold, &cast->arms[i].expr, tab);
				*node = cast->arms[i].expr;
				return;
			}
			else if (cast->arms[i].patn->kind == PATN_IDENT)
			{
				Patn_Ident *ident = (Patn_Ident *)cast->arms[i].patn;
				if (!STR_EQUAL(ident->name, STR_STATIC("_")))
				{
					Expr_Node *value;
					bool shad = expr_tab_get(tab, ident->name, &value);
					expr_tab_insert(tab, ident->name, cast->value);

					folder_expr(fold, &cast->arms[i].expr, tab);
					if (shad)
					{
						expr_tab_insert(tab, ident->name, value);
					}
					else
					{
						expr_tab_remove(tab, ident->name);
					}
				}
				*node = cast->arms[i].expr;
				return;
			}
		}
	}
	else if (cast->value->kind == EXPR_TUPLE)
	{
		Expr_Tuple *tuple = (Expr_Tuple *)cast->value;
		bool consts[tuple->len];

		for (size_t i = 0; i < tuple->len; ++i)
		{
			consts[i] = folder_const(tuple->items[i]) || tuple->items[i]->kind == EXPR_IDENT;
		}

		Expr_Tab tmp;
		expr_tab_init(&tmp, tab->seed);

		bool match = true;
		for (size_t i = 0; i < cast->len; ++i)
		{
			expr_tab_union(tab, &tmp);
			if (MAC_LIKELY(cast->arms[i].patn->kind == PATN_TUPLE))
			{
				Patn_Tuple *patn = (Patn_Tuple *)cast->arms[i].patn;
				for (size_t j = 0; j < tuple->len; ++j)
				{
					if (patn->items[j]->kind == PATN_CONST)
					{
						match = consts[j] &&
								const_equal(MAC_CAST(Patn_Const *, patn->items[j])->value,
													MAC_CAST(Expr_Const *, tuple->items[j])->value);
					}
					else if (patn->items[j]->kind == PATN_IDENT)
					{
						Patn_Ident *ident = (Patn_Ident *)patn->items[j];

						// TODO: Check for side effects
						// The wildcard `_` discards the value, even if not constant.
						//
						// Substituting the matching identifier with the value node
						// may also cause unexpected behaviour due to code duplication
						// in the presence of side effects.
						if (!STR_EQUAL(ident->name, STR_STATIC("_")))
						{
							if (consts[j])
							{
								expr_tab_insert(&tmp, ident->name, tuple->items[j]);
							}
							else
							{
								match = false;
							}
						}
					}

					if (!match)
					{
						break;
					}
				}

				if (match)
				{
					folder_expr(fold, &cast->arms[i].expr, &tmp);
					expr_tab_free(&tmp);

					*node = cast->arms[i].expr;
					return;
				}
			}
			expr_tab_reset(&tmp);
		}
		expr_tab_free(&tmp);
	}

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_expr(fold, &cast->arms[i].expr, tab);
		if (MAC_UNLIKELY(!patn_refutable(cast->arms[i].patn)))
		{
			cast->len = i + 1;
			break;
		}
	}

	// TODO: Improve pattern matching folding
	// Notably add support for nested constant tuple.
	// Also improve warnings for unreachable arms.
}

static MAC_HOT void
folder_let(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Let *cast = (Expr_Let *)*node;
	folder_expr(fold, &cast->value, tab);

	if (MAC_UNLIKELY(STR_EQUAL(cast->name, STR_STATIC("_"))))
	{
		// TODO: Check for side effects
		// Eliminating the let expression when the bound value is impossible to use
		// will yield incorrect results in case of side effects.
		//
		// However, since side effects can only be the results of impure function
		// applications, not eliminating if the let bound value contains an impure
		// function application (or in case of function application altogether),
		// will keep correctly the behaviour of the program.
		*node = cast->expr;
	}
	else if (folder_const(cast->value) || cast->value->kind == EXPR_IDENT)
	{
		Expr_Node *value;
		bool shad = expr_tab_get(tab, cast->name, &value);
		expr_tab_insert(tab, cast->name, cast->value);

		folder_expr(fold, &cast->expr, tab);
		*node = cast->expr;

		if (shad)
		{
			expr_tab_insert(tab, cast->name, value);
		}
		else
		{
			expr_tab_remove(tab, cast->name);
		}
	}
}

static MAC_HOT void
folder_fun(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Fun *cast = (Expr_Fun *)*node;
	folder_expr(fold, &cast->body, tab);
	folder_expr(fold, &cast->expr, tab);
}

static MAC_HOT void
folder_lambda(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Lambda *cast = (Expr_Lambda *)*node;
	folder_expr(fold, &cast->body, tab);
}

static MAC_HOT void
folder_app(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_App *cast = (Expr_App *)*node;
	folder_expr(fold, &cast->fun, tab);

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_expr(fold, &cast->args[i], tab);
	}

	// TODO: Function inlining
	// If the function being applied is made up only of constant
	// operations, function inlining and further folding are possible.
	//
	// However a dedicated optimization backend for inlining is preferable.
}

static MAC_HOT void
folder_ident(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Ident *cast = (Expr_Ident *)*node;

	Expr_Node *value;
	if (expr_tab_get(tab, cast->name, &value))
	{
		if (MAC_LIKELY(folder_const(value)))
		{
			*node = folder_const_copy(fold, MAC_CAST(Expr_Const *, value));
			(*node)->loc = cast->node.loc;
		}
		else if (value->kind == EXPR_IDENT)
		{
			cast->name = MAC_CAST(Expr_Ident *, value)->name;
		}
		else
		{
			// NOTE: Check for side effects and code duplication
			*node = value;
			folder_warn(fold, cast->node.loc, STR_STATIC("Substituted value is not constant"));
		}
	}
}

static MAC_HOT void
folder_tuple(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Tuple *cast = (Expr_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_expr(fold, &cast->items[i], tab);
	}
}

static MAC_HOT void
folder_expr(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	switch ((*node)->kind)
	{
		case EXPR_IF:
			folder_if(fold, node, tab);
			break;

		case EXPR_MATCH:
			folder_match(fold, node, tab);
			break;

		case EXPR_LET:
			folder_let(fold, node, tab);
			break;

		case EXPR_FUN:
			folder_fun(fold, node, tab);
			break;

		case EXPR_LAMBDA:
			folder_lambda(fold, node, tab);
			break;

		case EXPR_APP:
			folder_app(fold, node, tab);
			break;

		case EXPR_IDENT:
			folder_ident(fold, node, tab);
			break;

		case EXPR_TUPLE:
			folder_tuple(fold, node, tab);
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_HOT void
folder_decl(Folder *fold, Decl_Node *node, Expr_Tab *tab)
{
	switch (node->kind)
	{
		case DECL_LET:
			folder_expr(fold, &MAC_CAST(Decl_Let *, node)->value, tab);
			if (folder_const(MAC_CAST(Decl_Let *, node)->value) ||
				MAC_CAST(Decl_Let *, node)->value->kind == EXPR_IDENT)
			{
				expr_tab_insert(tab, MAC_CAST(Decl_Let *, node)->name,
								MAC_CAST(Decl_Let *, node)->value);
			}
			break;

		case DECL_FUN:
			folder_expr(fold, &MAC_CAST(Decl_Fun *, node)->body, tab);
			break;

		case DECL_DATA:
			break;

		case DECL_TYPE:
			break;
	}
}

void
folder_init(Folder *fold, Arena *arena, Hash_Default seed)
{
	fold->arena = arena;
	fold->seed = seed;
	fold->err = NULL;
}

MAC_HOT void
folder_ast(Folder *fold, Ast_Top *ast, Error_List *err)
{
	Expr_Tab tab;
	expr_tab_init(&tab, fold->seed);

	fold->err = err;
	for (size_t i = 0; i < ast->len; ++i)
	{
		Decl_Node *node = ast->decls[i];
		folder_decl(fold, node, &tab);
	}
	expr_tab_free(&tab);
}
