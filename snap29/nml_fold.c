#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "nml_ast.h"
#include "nml_const.h"
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

static const Keyword ops[] = {
#define TOK(name, str, id, _) { STR_STATIC(str), TOK_ ## name },
#define OPER
#include "nml_tok.h"
#undef OPER
#undef TOK
	{ STR_STATIC(""),	TOK_EOF },
};

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
			*node = cast->arms[0].expr;
		}
		else
		{
			// TODO: Check for side effects
			*node = expr_let_new(fold->arena, cast->node.loc, MAC_CAST(Patn_Ident *, cast->arms[0].patn)->name,
								SCHEME_EMPTY(TYPE_NONE), cast->value, cast->arms[0].expr);

			folder_type(fold, *node, cast->arms[0].expr->type);
			folder_expr(fold, node, tab);
		}

		folder_warn(fold, cast->arms[0].patn->loc, STR_STATIC("First pattern in match is a wildcard"));
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
		if (MAC_UNLIKELY(!pattern_refutable(cast->arms[i].patn)))
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
		if (cast->expr != NULL)
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
		else
		{
			expr_tab_insert(tab, cast->name, cast->value);
		}
	}

	// TODO: Fix dead code elimination
	// For unused let which do not have a constant value
}

static MAC_HOT void
folder_fun(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Fun *cast = (Expr_Fun *)*node;
	folder_expr(fold, &cast->body, tab);

	if (cast->expr != NULL)
	{
		folder_expr(fold, &cast->expr, tab);
	}
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

	// TODO: Improve builtin functions folding
	if (cast->fun->kind == EXPR_IDENT && folder_const(cast->args[0]))
	{
		if (cast->len == 1)
		{
			Expr_Const *arg = MAC_CAST(Expr_Const *, cast->args[0]);
			if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->fun)->name, STR_STATIC("neg")))
			{
				arg->value.c_int = -arg->value.c_int;
				*node = cast->args[0];
			}
			else if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->fun)->name, STR_STATIC("fneg")))
			{
				arg->value.c_float = -arg->value.c_float;
				*node = cast->args[0];
			}
			else if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->fun)->name, STR_STATIC("not")))
			{
				arg->value.c_bool = !arg->value.c_bool;
				*node = cast->args[0];
			}
		}
		else if (cast->len == 2 && folder_const(cast->args[1]))
		{
			Expr_Ident *ident = (Expr_Ident *)cast->fun;
			if (true ||  !isalpha(ident->name.str[0]))
			{
				for (size_t i = 0; ops[i].kind != TOK_EOF; ++i)
				{
					if (STR_EQUAL(ops[i].name, ident->name))
					{
						*node = expr_binary_new(fold->arena, cast->node.loc, ops[i].kind, cast->args[0], cast->args[1]);
						folder_expr(fold, node, tab);
						return;
					}
				}
			}
		}
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
folder_binary(Folder *fold, Expr_Node **node, Expr_Tab *tab)
{
	Expr_Binary *cast = (Expr_Binary *)*node;

	folder_expr(fold, &cast->rhs, tab);
	folder_expr(fold, &cast->lhs, tab);

	if (MAC_UNLIKELY(cast->node.type == TYPE_UNIT))
	{
		if (cast->op == TOK_EQ)
		{
			bool cond = true;
			*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
			folder_type(fold, *node, TYPE_BOOL);
		}
		else if (cast->op == TOK_NE)
		{
			bool cond = false;
			*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
			folder_type(fold, *node, TYPE_BOOL);
		}
	}
	else if (folder_const(cast->rhs))
	{
		Expr_Const *rhs = MAC_CAST(Expr_Const *, cast->rhs);

		if (folder_const(cast->lhs))
		{
			Expr_Const *lhs = MAC_CAST(Expr_Const *, cast->lhs);
			bool cond;

			switch (cast->op)
			{
				case TOK_PLUS:
					rhs->value.c_int += lhs->value.c_int;
					*node = (Expr_Node *)rhs;
					break;

				case TOK_MINUS:
					rhs->value.c_int -= lhs->value.c_int;
					*node = (Expr_Node *)rhs;
					break;

				case TOK_STAR:
					rhs->value.c_int *= lhs->value.c_int;
					*node = (Expr_Node *)rhs;
					break;

				case TOK_SLASH:
					// NOTE: Warn on division by zero
					if (MAC_LIKELY(lhs->value.c_int != 0))
					{
						rhs->value.c_int /= lhs->value.c_int;
						*node = (Expr_Node *)rhs;
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant division by zero"));
					}
					break;

				case TOK_PERC:
					// NOTE: Warn on remainder by zero
					if (MAC_LIKELY(lhs->value.c_int != 0))
					{
						rhs->value.c_int %= lhs->value.c_int;
						*node = (Expr_Node *)rhs;
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant remainder by zero"));
					}
					break;

				case TOK_PLUSF:
					rhs->value.c_float += lhs->value.c_float;
					*node = (Expr_Node *)rhs;
					break;

				case TOK_MINUSF:
					rhs->value.c_float -= lhs->value.c_float;
					*node = (Expr_Node *)rhs;
					break;

				case TOK_STARF:
					rhs->value.c_float *= lhs->value.c_float;
					*node = (Expr_Node *)rhs;
					break;

				case TOK_SLASHF:
					// NOTE: Warn on division by zero
					if (MAC_LIKELY(lhs->value.c_float != 0))
					{
						rhs->value.c_float /= lhs->value.c_float;
						*node = (Expr_Node *)rhs;
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant division by zero"));
					}
					break;

				case TOK_PERCF:
					// NOTE: Warn on remainder by zero
					if (MAC_LIKELY(lhs->value.c_float != 0))
					{
						// TODO: Float modulo
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant remainder by zero"));
					}
					break;

				case TOK_GT:
					if (rhs->node.type == TYPE_INT)
					{
						cond = rhs->value.c_int > lhs->value.c_int;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					else if (rhs->node.type == TYPE_FLOAT)
					{
						cond = rhs->value.c_float > lhs->value.c_float;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					break;

				case TOK_LT:
					if (rhs->node.type == TYPE_INT)
					{
						cond = rhs->value.c_int < lhs->value.c_int;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					else if (rhs->node.type == TYPE_FLOAT)
					{
						cond = rhs->value.c_float < lhs->value.c_float;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					break;

				case TOK_GTE:
					if (rhs->node.type == TYPE_INT)
					{
						cond = rhs->value.c_int >= lhs->value.c_int;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					else if (rhs->node.type == TYPE_FLOAT)
					{
						cond = rhs->value.c_float >= lhs->value.c_float;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					break;

				case TOK_LTE:
					if (rhs->node.type == TYPE_INT)
					{
						cond = rhs->value.c_int <= lhs->value.c_int;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					else if (rhs->node.type == TYPE_FLOAT)
					{
						cond = rhs->value.c_float <= lhs->value.c_float;
						*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
						folder_type(fold, *node, TYPE_BOOL);
					}
					break;

				case TOK_EQ:
					cond = const_equal(rhs->value, lhs->value);
					*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				case TOK_NE:
					cond = !const_equal(rhs->value, lhs->value);
					*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				case TOK_AMPAMP:
					cond = rhs->value.c_bool && lhs->value.c_bool;
					*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				case TOK_BARBAR:
					cond = rhs->value.c_bool || lhs->value.c_bool;
					*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				default:
					break;
			}
		}
		else
		{
			switch (cast->op)
			{
				case TOK_PLUS:
				case TOK_MINUS:
					if (MAC_UNLIKELY(rhs->value.c_int == 0))
					{
						*node = cast->lhs;
					}
					break;

				case TOK_PLUSF:
				case TOK_MINUSF:
					if (MAC_UNLIKELY(rhs->value.c_float == 0))
					{
						*node = cast->lhs;
					}
					break;

				case TOK_STAR:
					if (MAC_UNLIKELY(rhs->value.c_int == 0))
						{
						// TODO: Check for side effects
						// The elimination of the `lhs` of this experssion
						// will yield incorrect results in case of side effects.
						//
						// However, since side effects can only be the results of impure function
						// applications, not eliminating if the `lhs` contains an impure
						// function application (or in case of function application altogether),
						// will keep correctly the behaviour of the program.
						*node = cast->rhs;
					}
					else if (MAC_UNLIKELY(rhs->value.c_int == 1))
					{
						*node = cast->lhs;
					}
					break;

				case TOK_STARF:
					if (MAC_UNLIKELY(rhs->value.c_float == 0))
					{
						// TODO: Check for side effects
						// The elimination of the `lhs` of this experssion
						// will yield incorrect results in case of side effects.
						//
						// However, since side effects can only be the results of impure function
						// applications, not eliminating if the `lhs` contains an impure
						// function application (or in case of function application altogether),
						// will keep correctly the behaviour of the program.
						*node = cast->rhs;
					}
					else if (MAC_UNLIKELY(rhs->value.c_float == 1))
					{
						*node = cast->lhs;
					}
					break;

				case TOK_AMPAMP:
					if (!rhs->value.c_bool)
					{
						// TODO: Check for side effects
						// The elimination of the `lhs` of this experssion
						// will yield incorrect results in case of side effects.
						//
						// However, since side effects can only be the results of impure function
						// applications, not eliminating if the `lhs` contains an impure
						// function application (or in case of function application altogether),
						// will keep correctly the behaviour of the program.
						*node = cast->rhs;
					}
					else
					{
						*node = cast->lhs;
					}
					break;

				case TOK_BARBAR:
					if (rhs->value.c_bool)
					{
						// TODO: Check for side effects
						// The elimination of the `lhs` of this experssion
						// will yield incorrect results in case of side effects.
						//
						// However, since side effects can only be the results of impure function
						// applications, not eliminating if the `lhs` contains an impure
						// function application (or in case of function application altogether),
						// will keep correctly the behaviour of the program.
						*node = cast->rhs;
					}
					else
					{
						*node = cast->lhs;
					}
					break;

				default:
					break;
			}
		}
	}
	else if (folder_const(cast->lhs))
	{
		Expr_Const *lhs = MAC_CAST(Expr_Const *, cast->lhs);

		switch (cast->op)
		{
			case TOK_PLUS:
			case TOK_MINUS:
				if (MAC_UNLIKELY(lhs->value.c_int == 0))
				{
					*node = cast->rhs;
				}
				break;

			case TOK_PLUSF:
			case TOK_MINUSF:
				if (MAC_UNLIKELY(lhs->value.c_float == 0))
				{
					*node = cast->rhs;
				}
				break;

			case TOK_STAR:
				if (MAC_UNLIKELY(lhs->value.c_int == 0))
				{
					// TODO: Check for side effects
					// The elimination of the `rhs` of this experssion
					// will yield incorrect results in case of side effects.
					//
					// However, since side effects can only be the results of impure function
					// applications, not eliminating if the `rhs` contains an impure
					// function application (or in case of function application altogether),
					// will keep correctly the behaviour of the program.
					*node = cast->lhs;
				}
				else if (MAC_UNLIKELY(lhs->value.c_int == 1))
				{
					*node = cast->rhs;
				}
				break;

			case TOK_STARF:
				if (MAC_UNLIKELY(lhs->value.c_float == 0))
				{
					// TODO: Check for side effects
					// The elimination of the `rhs` of this experssion
					// will yield incorrect results in case of side effects.
					//
					// However, since side effects can only be the results of impure function
					// applications, not eliminating if the `rhs` contains an impure
					// function application (or in case of function application altogether),
					// will keep correctly the behaviour of the program.
					*node = cast->lhs;
				}
				else if (MAC_UNLIKELY(lhs->value.c_float == 1))
				{
					*node = cast->rhs;
				}
				break;

			case TOK_AMPAMP:
				if (!lhs->value.c_bool)
				{
					// TODO: Check for side effects
					// The elimination of the `rhs` of this experssion
					// will yield incorrect results in case of side effects.
					//
					// However, since side effects can only be the results of impure function
					// applications, not eliminating if the `rhs` contains an impure
					// function application (or in case of function application altogether),
					// will keep correctly the behaviour of the program.
					*node = cast->lhs;
				}
				else
				{
					*node = cast->rhs;
				}
				break;

			case TOK_BARBAR:
				if (lhs->value.c_bool)
				{
					// TODO: Check for side effects
					// The elimination of the `rhs` of this experssion
					// will yield incorrect results in case of side effects.
					//
					// However, since side effects can only be the results of impure function
					// applications, not eliminating if the `rhs` contains an impure
					// function application (or in case of function application altogether),
					// will keep correctly the behaviour of the program.
					*node = cast->lhs;
				}
				else
				{
					*node = cast->rhs;
				}
				break;

			default:
				break;
		}
	}
	else if (cast->rhs->kind == EXPR_IDENT && cast->lhs->kind == EXPR_IDENT)
	{
		if (cast->op == TOK_MINUS)
		{
			if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->rhs)->name, MAC_CAST(Expr_Ident *, cast->lhs)->name))
			{
				int32_t zero = 0;
				*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_INT, &zero));
				folder_type(fold, *node, TYPE_INT);
			}
		}
		else if (cast->op == TOK_MINUSF)
		{
			if (STR_EQUAL(MAC_CAST(Expr_Ident *, cast->rhs)->name, MAC_CAST(Expr_Ident *, cast->lhs)->name))
			{
				double zero = 0;
				*node = expr_const_new(fold->arena, cast->node.loc, const_new(CONST_FLOAT, &zero));
				folder_type(fold, *node, TYPE_FLOAT);
			}
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

		case EXPR_APP:
			folder_app(fold, node, tab);
			break;

		case EXPR_IDENT:
			folder_ident(fold, node, tab);
			break;

		case EXPR_BINARY:
			folder_binary(fold, node, tab);
			break;

		case EXPR_TUPLE:
			folder_tuple(fold, node, tab);
			break;

		case EXPR_CONST:
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
		if (MAC_LIKELY(ast->items[i].kind == AST_EXPR))
		{
			Expr_Node **node = (Expr_Node **)&ast->items[i].node;
			bool dead = true;

			if (((*node)->kind == EXPR_FUN && MAC_CAST(Expr_Fun *, *node)->expr == NULL) ||
				((*node)->kind == EXPR_LET && MAC_CAST(Expr_Let *, *node)->expr == NULL))
			{
				folder_expr(fold, node, &tab);
				dead = *node == NULL;
			}

			if (dead)
			{
				if (--ast->len != i)
				{
					memmove(ast->items + i, ast->items + i + 1,
							(ast->len - i) * sizeof(Ast_Node *));
					--i;
				}
			}
		}
	}
	expr_tab_free(&tab);
}
