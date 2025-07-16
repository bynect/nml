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

static MAC_HOT void folder_expr(Folder *fold, Expr_Node **node, Expr_Tab *def);

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

static MAC_CONST MAC_INLINE inline bool
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
folder_if(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_If *cast = (Expr_If *)*node;
	folder_expr(fold, &cast->cond, def);

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

		folder_expr(fold, node, def);
		return;
	}

	folder_expr(fold, &cast->b_then, def);
	folder_expr(fold, &cast->b_else, def);

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
folder_match(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_Match *cast = (Expr_Match *)*node;
	folder_expr(fold, &cast->value, def);

	if (MAC_UNLIKELY(cast->arms[0].patn->kind == PATN_IDENT))
	{
		// NOTE: If the first pattern is a wildcard directly substitute it
		if (folder_const(cast->value) || cast->value->kind == EXPR_IDENT)
		{
			Patn_Ident *ident = (Patn_Ident *)cast->arms[0].patn;
			if (!STR_EQUAL(ident->name, STR_STATIC("_")))
			{
				Expr_Node *value;
				bool shad = expr_tab_get(def, ident->name, &value);
				expr_tab_insert(def, ident->name, cast->value);

				folder_expr(fold, &cast->arms[0].expr, def);
				if (shad)
				{
					expr_tab_insert(def, ident->name, value);
				}
				else
				{
					expr_tab_remove(def, ident->name);
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
			folder_expr(fold, node, def);
		}
		return;
	}
	else if (MAC_UNLIKELY(cast->arms[0].patn->kind == PATN_CONST &&
			MAC_CAST(Patn_Const *, cast->arms[0].patn)->value.kind == CONST_UNIT))
	{
		*node = cast->arms[0].expr;
		folder_expr(fold, node, def);
		return;
	}
	else if (folder_const(cast->value))
	{
		for (size_t i = 0; i < cast->len; ++i)
		{
			if (cast->arms[i].patn->kind == PATN_CONST &&
				const_equal(MAC_CAST(Expr_Const *, cast->value)->value, MAC_CAST(Patn_Const *, cast->arms[i].patn)->value))
			{
				folder_expr(fold, &cast->arms[i].expr, def);
				*node = cast->arms[i].expr;
				return;
			}
			else if (cast->arms[i].patn->kind == PATN_IDENT)
			{
				Patn_Ident *ident = (Patn_Ident *)cast->arms[i].patn;
				if (!STR_EQUAL(ident->name, STR_STATIC("_")))
				{
					Expr_Node *value;
					bool shad = expr_tab_get(def, ident->name, &value);
					expr_tab_insert(def, ident->name, cast->value);

					folder_expr(fold, &cast->arms[i].expr, def);
					if (shad)
					{
						expr_tab_insert(def, ident->name, value);
					}
					else
					{
						expr_tab_remove(def, ident->name);
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
		expr_tab_init(&tmp, def->seed);

		bool match = true;
		for (size_t i = 0; i < cast->len; ++i)
		{
			expr_tab_union(def, &tmp);
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
		folder_expr(fold, &cast->arms[i].expr, def);
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
folder_let(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_Let *cast = (Expr_Let *)*node;
	folder_expr(fold, &cast->value, def);

	if (folder_const(cast->expr))
	{
		*node = cast->expr;
	}
	else if (cast->expr->kind == EXPR_IDENT)
	{
		if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->expr)->name, cast->name))
		{
			*node = cast->value;
		}
		else
		{
			*node = cast->expr;
		}
	}
	else if (MAC_UNLIKELY(STR_EQUAL(cast->name, STR_STATIC("_"))))
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
		bool shad = expr_tab_get(def, cast->name, &value);
		expr_tab_insert(def, cast->name, cast->value);

		folder_expr(fold, &cast->expr, def);
		*node = cast->expr;

		if (shad)
		{
			expr_tab_insert(def, cast->name, value);
		}
		else
		{
			expr_tab_remove(def, cast->name);
		}
	}
}

static MAC_HOT void
folder_fun(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_Fun *cast = (Expr_Fun *)*node;
	folder_expr(fold, &cast->body, def);
	folder_expr(fold, &cast->expr, def);

	if (folder_const(cast->expr))
	{
		*node = cast->expr;
	}
	else if (cast->expr->kind == EXPR_IDENT)
	{
		if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->expr)->name, cast->name))
		{
			*node = expr_lambda_new(fold->arena, cast->node.loc, cast->pars, cast->len, cast->body);
		}
		else
		{
			*node = cast->expr;
		}
	}
}

static MAC_HOT void
folder_lambda(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_Lambda *cast = (Expr_Lambda *)*node;
	folder_expr(fold, &cast->body, def);

	// TODO: Fix lambda merging algorithm
	// The following implementation is highly unoptimal and
	// allocates new parameters for every lambda merging.

	// TODO: Move to merge pass
//	if (cast->body->kind == EXPR_LAMBDA)
//	{
//		Expr_Lambda *body = (Expr_Lambda *)cast->body;
//		Sized_Str *pars = arena_alloc(fold->arena, (cast->len + body->len) * sizeof(Sized_Str));
//
//		memcpy(pars, cast->pars, cast->len * sizeof(Sized_Str));
//		memcpy(pars + cast->len, body->pars, body->len * sizeof(Sized_Str));
//
//		cast->pars = pars;
//		cast->len = (cast->len + body->len);
//		cast->body = body->body;
//	}
}

static MAC_HOT void
folder_app(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_App *cast = (Expr_App *)*node;
	folder_expr(fold, &cast->fun, def);

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_expr(fold, &cast->args[i], def);
	}

	// TODO: Function inlining
	// If the function being applied is made up only of constant
	// operations, function inlining and further folding are possible.
	//
	// However a dedicated optimization backend for inlining is preferable.

	// TODO: Move to an inline pass
	//if (cast->fun->kind == EXPR_LAMBDA)
	//{
	//	Expr_Lambda *fun = (Expr_Lambda *)cast->fun;
	//	size_t len = 0;

	//	while (len < cast->len && len < fun->len)
	//	{
	//		if (!folder_const(cast->args[len]))
	//		{
	//			break;
	//		}
	//		++len;
	//	}

	//	if (len != 0)
	//	{
	//		Expr_Node *tmp[len];
	//		bool shad[len];

	//		for (size_t i = 0; i < len; ++i)
	//		{
	//			shad[i] = expr_tab_get(def, fun->pars[i], &tmp[i]);
	//			expr_tab_insert(def, fun->pars[i], cast->args[i]);
	//		}

	//		folder_expr(fold, &cast->fun, def);
	//		for (size_t i = 0; i < len; ++i)
	//		{
	//			if (shad[i])
	//			{
	//				expr_tab_insert(def, fun->pars[i], tmp[i]);
	//			}
	//			else
	//			{
	//				expr_tab_remove(def, fun->pars[i]);
	//			}
	//		}

	//		if (len < fun->len)
	//		{
	//			fun->len -= len;
	//			memmove(fun->pars, fun->pars + len, fun->len * sizeof(Sized_Str));
	//		}
	//		else
	//		{
	//			cast->fun = fun->body;
	//		}

	//		if (len < cast->len)
	//		{
	//			cast->len -= len;
	//			memmove(cast->args, cast->args + len, cast->len * sizeof(Expr_Node *));
	//		}
	//		else
	//		{
	//			*node = cast->fun;
	//		}
	//	}
	//}
	if (cast->fun->kind == EXPR_IDENT)
	{
		Sized_Str fun = MAC_CAST(Expr_Ident *, cast->fun)->name;
		if (cast->len == 2 && folder_const(cast->args[0]) && folder_const(cast->args[1]))
		{
			if (fun.len > 10 && !memcmp(fun.str, "Intrinsic.", 10))
			{
				Expr_Const *rhs = (Expr_Const *)cast->args[0];
				Expr_Const *lhs = (Expr_Const *)cast->args[1];

				fun = STR_WLEN(fun.str + 10, fun.len - 10);
				if (STR_EQUAL(fun, STR_STATIC("add")))
				{
					rhs->value.c_int += lhs->value.c_int;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("sub")))
				{
					rhs->value.c_int -= lhs->value.c_int;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("mul")))
				{
					rhs->value.c_int *= lhs->value.c_int;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("div")))
				{
					if (lhs->value.c_int != 0)
					{
						rhs->value.c_int /= lhs->value.c_int;
						*node = cast->args[0];
					}
				}
				else if (STR_EQUAL(fun, STR_STATIC("rem")))
				{
					if (lhs->value.c_int != 0)
					{
						rhs->value.c_int %= lhs->value.c_int;
						*node = cast->args[0];
					}
				}
				else if (STR_EQUAL(fun, STR_STATIC("fadd")))
				{
					rhs->value.c_float += lhs->value.c_float;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("fsub")))
				{
					rhs->value.c_float -= lhs->value.c_float;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("fmul")))
				{
					rhs->value.c_float *= lhs->value.c_float;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("fdiv")))
				{
					if (lhs->value.c_float != 0)
					{
						rhs->value.c_float /= lhs->value.c_float;
						*node = cast->args[0];
					}
				}
				else if (STR_EQUAL(fun, STR_STATIC("frem")))
				{
					// TODO
				}
			}
		}
		else if (cast->len == 1 && folder_const(cast->args[0]))
		{
			if (fun.len > 10 && !memcmp(fun.str, "Intrinsic.", 10))
			{
				Expr_Const *rhs = (Expr_Const *)cast->args[0];

				fun = STR_WLEN(fun.str + 10, fun.len - 10);
				if (STR_EQUAL(fun, STR_STATIC("neg")))
				{
					rhs->value.c_int = -rhs->value.c_int;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("fneg")))
				{
					rhs->value.c_float = -rhs->value.c_float;
					*node = cast->args[0];
				}
				else if (STR_EQUAL(fun, STR_STATIC("not")))
				{
					rhs->value.c_bool = !rhs->value.c_bool;
					*node = cast->args[0];
				}
			}
		}
	}
}

static MAC_HOT void
folder_ident(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_Ident *cast = (Expr_Ident *)*node;

	Expr_Node *value;
	if (expr_tab_get(def, cast->name, &value))
	{
		if (MAC_LIKELY(folder_const(value)))
		{
			*node = folder_const_copy(fold, MAC_CAST(Expr_Const *, value));
			(*node)->loc = cast->node.loc;
		}
		else if (value->kind == EXPR_IDENT)
		{
			// FIXME: Fragile with redefinitions
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
folder_tuple(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	Expr_Tuple *cast = (Expr_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_expr(fold, &cast->items[i], def);
	}
}

static MAC_HOT void
folder_expr(Folder *fold, Expr_Node **node, Expr_Tab *def)
{
	switch ((*node)->kind)
	{
		case EXPR_IF:
			folder_if(fold, node, def);
			break;

		case EXPR_MATCH:
			folder_match(fold, node, def);
			break;

		case EXPR_LET:
			folder_let(fold, node, def);
			break;

		case EXPR_FUN:
			folder_fun(fold, node, def);
			break;

		case EXPR_LAMBDA:
			folder_lambda(fold, node, def);
			break;

		case EXPR_APP:
			folder_app(fold, node, def);
			break;

		case EXPR_IDENT:
			folder_ident(fold, node, def);
			break;

		case EXPR_TUPLE:
			folder_tuple(fold, node, def);
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_HOT void
folder_decl(Folder *fold, Decl_Node *node, Expr_Tab *def)
{
	switch (node->kind)
	{
		case DECL_LET:
			folder_expr(fold, &MAC_CAST(Decl_Let *, node)->value, def);
			if (folder_const(MAC_CAST(Decl_Let *, node)->value) ||
				MAC_CAST(Decl_Let *, node)->value->kind == EXPR_IDENT)
			{
				expr_tab_insert(def, MAC_CAST(Decl_Let *, node)->name,
								MAC_CAST(Decl_Let *, node)->value);
			}
			break;

		case DECL_FUN:
			folder_expr(fold, &MAC_CAST(Decl_Fun *, node)->body, def);
			break;

		case DECL_DATA:
			break;

		case DECL_TYPE:
			break;
	}
}

void
folder_init(Folder *fold, Arena *arena, Hash_Default seed, Expr_Tab *def)
{
	fold->arena = arena;
	fold->err = NULL;

	fold->seed = seed;
	if (def == NULL)
	{
		def = arena_alloc(arena, sizeof(Expr_Tab));
		expr_tab_init(def, seed);
	}
	fold->def = def;
}

MAC_HOT void
folder_pass(Folder *fold, Ast_Top *ast, Error_List *err)
{
	fold->err = err;
	for (size_t i = 0; i < ast->len; ++i)
	{
		Decl_Node *node = ast->decls[i];
		folder_decl(fold, node, fold->def);
	}
}
