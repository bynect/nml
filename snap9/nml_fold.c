#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "nml_ast.h"
#include "nml_const.h"
#include "nml_err.h"
#include "nml_fold.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_patn.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_type.h"

#define CAST_IDENT(node)	((Ast_Ident *)node)
#define CAST_CONST(node)	((Ast_Const *)node)

static MAC_HOT void folder_node(Folder *fold, Ast_Node **node, Ast_Tab *tab);

static MAC_INLINE inline void
folder_warn(Folder *fold, Location loc, Sized_Str msg)
{
	error_append(fold->err, fold->arena, msg, loc, ERR_WARN);
}

static MAC_INLINE inline void
folder_type(Folder *fold, Ast_Node *node, Type type)
{
	if (MAC_UNLIKELY(node->type != TYPE_NONE && node->type != type))
	{
		folder_warn(fold, node->loc, STR_STATIC("Mismatched types"));
	}
	node->type = type;
}

static MAC_CONST inline bool
folder_const(Ast_Node *node)
{
	return node->kind == AST_CONST;
}

static MAC_INLINE inline Ast_Node *
folder_const_copy(Folder *fold, Ast_Const *node)
{
	Ast_Const *tmp = arena_alloc(fold->arena, sizeof(Ast_Const));
	memcpy(tmp, node, sizeof(Ast_Const));
	return (Ast_Node *)tmp;
}

static MAC_HOT void
folder_if(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_If *cast = (Ast_If *)*node;

	folder_node(fold, &cast->cond, tab);
	folder_node(fold, &cast->then, tab);

	if (cast->m_else != NULL)
	{
		folder_node(fold, &cast->m_else, tab);
	}

	if (folder_const(cast->cond))
	{
		if (CAST_CONST(cast->cond)->value.c_bool)
		{
			*node = cast->then;
			folder_warn(fold, cast->cond->loc, STR_STATIC("Condition is always true"));
		}
		else if (!CAST_CONST(cast->cond)->value.c_bool)
		{
			if (cast->m_else != NULL)
			{
				*node = cast->m_else;
			}
			else
			{
				*node = ast_const_new(fold->arena, cast->then->loc,	const_new(CONST_UNIT, NULL));
				folder_type(fold, *node, TYPE_UNIT);
			}
			folder_warn(fold, cast->cond->loc, STR_STATIC("Condition is always false"));
		}
	}
	else if (folder_const(cast->then))
	{
		if (cast->m_else == NULL)
		{
			if (CAST_CONST(cast->then)->value.kind == CONST_UNIT)
			{
				*node = cast->then;
			}
		}
		else if (folder_const(cast->m_else) &&
				const_equal(CAST_CONST(cast->then)->value, CAST_CONST(cast->m_else)->value))
		{
			*node = cast->then;
		}
	}
}

static MAC_HOT void
folder_match(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Match *cast = (Ast_Match *)*node;

	folder_node(fold, &cast->value, tab);
	if (MAC_UNLIKELY(cast->arms[0].patn->kind == PATN_IDENT))
	{
		// NOTE: If the first pattern is a wildcard directly substitute it
		if (folder_const(cast->value) || cast->value->kind == AST_IDENT)
		{
			Patn_Ident *ident = (Patn_Ident *)cast->arms[0].patn;
			if (!STR_EQUAL(ident->name, STR_STATIC("_")))
			{
				Ast_Node *value;
				bool shad = ast_tab_get(tab, ident->name, &value);
				ast_tab_insert(tab, fold->arena, ident->name, cast->value);

				folder_node(fold, &cast->arms[0].expr, tab);
				if (shad)
				{
					ast_tab_insert(tab, fold->arena, ident->name, value);
				}
				else
				{
					ast_tab_remove(tab, ident->name);
				}
			}
			*node = cast->arms[0].expr;
		}
		else
		{
			// TODO: Check for side effects
			*node = ast_let_new(fold->arena, cast->node.loc, ((Patn_Ident *)cast->arms[0].patn)->name,
								TYPE_NONE, cast->value, cast->arms[0].expr);

			folder_type(fold, *node, cast->arms[0].expr->type);
			folder_node(fold, node, tab);
		}

		folder_warn(fold, cast->arms[0].patn->loc, STR_STATIC("First pattern in match is a wildcard"));
		return;
	}
	else if (MAC_UNLIKELY(cast->arms[0].patn->kind == PATN_CONST &&
			((Patn_Const *)cast->arms[0].patn)->value.kind == CONST_UNIT))
	{
		*node = cast->arms[0].expr;
		folder_node(fold, node, tab);
		return;
	}
	else if (folder_const(cast->value))
	{
		for (size_t i = 0; i < cast->len; ++i)
		{
			if ((cast->arms[i].patn->kind == PATN_CONST) &&
				const_equal(CAST_CONST(cast->value)->value, ((Patn_Const *)cast->arms[i].patn)->value))
			{
				folder_node(fold, &cast->arms[i].expr, tab);
				*node = cast->arms[i].expr;
				return;
			}
			else if (cast->arms[i].patn->kind == PATN_IDENT)
			{
				Patn_Ident *ident = (Patn_Ident *)cast->arms[i].patn;
				if (!STR_EQUAL(ident->name, STR_STATIC("_")))
				{
					Ast_Node *value;
					bool shad = ast_tab_get(tab, ident->name, &value);
					ast_tab_insert(tab, fold->arena, ident->name, cast->value);

					folder_node(fold, &cast->arms[i].expr, tab);
					if (shad)
					{
						ast_tab_insert(tab, fold->arena, ident->name, value);
					}
					else
					{
						ast_tab_remove(tab, ident->name);
					}
				}
				*node = cast->arms[i].expr;
				return;
			}
		}
	}
	else if (cast->value->kind == AST_TUPLE)
	{
		Ast_Tuple *tuple = (Ast_Tuple *)cast->value;
		bool consts[tuple->len];

		for (size_t i = 0; i < tuple->len; ++i)
		{
			consts[i] = folder_const(tuple->items[i]) || tuple->items[i]->kind == AST_IDENT;
		}

		Ast_Tab tmp;
		ast_tab_init(&tmp, tab->seed);

		bool match = true;
		for (size_t i = 0; i < cast->len; ++i)
		{
			ast_tab_union(tab, &tmp, fold->arena);
			if (MAC_LIKELY(cast->arms[i].patn->kind == PATN_TUPLE))
			{
				Patn_Tuple *patn = (Patn_Tuple *)cast->arms[i].patn;
				for (size_t j = 0; j < tuple->len; ++j)
				{
					if (patn->items[j]->kind == PATN_CONST)
					{
						match = consts[j] &&
								const_equal(((Patn_Const *)patn->items[j])->value,
													CAST_CONST(tuple->items[j])->value);
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
								ast_tab_insert(&tmp, fold->arena, ident->name, tuple->items[j]);
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
					folder_node(fold, &cast->arms[i].expr, &tmp);
					*node = cast->arms[i].expr;
					return;
				}
			}
			ast_tab_reset(&tmp);
		}
	}

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_node(fold, &cast->arms[i].expr, tab);

		// NOTE: Remove unreachable patterns
		if (MAC_UNLIKELY(cast->arms[i].patn->kind == PATN_IDENT))
		{
			cast->len = i;
			break;
		}
	}

	// TODO: Improve pattern matching folding
	// Notably add support for nested constant tuple.
	// Also improve warnings for unreachable arms.
}

static MAC_HOT void
folder_let(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Let *cast = (Ast_Let *)*node;

	folder_node(fold, &cast->value, tab);
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
	else if (folder_const(cast->value) || cast->value->kind == AST_IDENT)
	{
		if (cast->expr != NULL)
		{
			Ast_Node *value;
			bool shad = ast_tab_get(tab, cast->name, &value);
			ast_tab_insert(tab, fold->arena, cast->name, cast->value);

			folder_node(fold, &cast->expr, tab);
			*node = cast->expr;

			if (shad)
			{
				ast_tab_insert(tab, fold->arena, cast->name, value);
			}
			else
			{
				ast_tab_remove(tab, cast->name);
			}
		}
		else
		{
			ast_tab_insert(tab, fold->arena, cast->name, cast->value);
		}
	}

	// TODO: Fix dead code elimination
	// For unused let which do not have a constant value
}

static MAC_HOT void
folder_fun(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Fun *cast = (Ast_Fun *)*node;

	folder_node(fold, &cast->body, tab);
	if (cast->expr != NULL)
	{
		folder_node(fold, &cast->expr, tab);
	}
}

static MAC_HOT void
folder_app(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_App *cast = (Ast_App *)*node;

	folder_node(fold, &cast->fun, tab);
	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_node(fold, &cast->args[i], tab);
	}

	// TODO: Function inlining
	// If the function being applied is made up only of constant
	// operations, function inlining and further folding are possible.
	//
	// However a dedicated optimization backend for inlining is preferable.
}

static MAC_HOT void
folder_ident(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Ident *cast = (Ast_Ident *)*node;

	Ast_Node *value;
	if (ast_tab_get(tab, cast->name, &value))
	{
		if (MAC_LIKELY(folder_const(value)))
		{
			*node = folder_const_copy(fold, CAST_CONST(value));
		}
		else if (value->kind == AST_IDENT)
		{
			cast->name = ((Ast_Ident *)value)->name;
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
folder_unary(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Unary *cast = (Ast_Unary *)*node;

	folder_node(fold, &cast->lhs, tab);
	if (folder_const(cast->lhs))
	{
		Ast_Const *lhs = CAST_CONST(cast->lhs);
		if (cast->op == TOK_MINUS)
		{
			if (lhs->value.kind == CONST_INT)
			{
				lhs->value.c_int = -lhs->value.c_int;
			}
			else if (lhs->value.kind == CONST_FLOAT)
			{
				lhs->value.c_float = -lhs->value.c_float;
			}
		}
		else if (cast->op == TOK_BANG)
		{
			lhs->value.c_bool = !lhs->value.c_bool;
		}
		*node = cast->lhs;
	}
}

static MAC_HOT void
folder_binary(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Binary *cast = (Ast_Binary *)*node;

	folder_node(fold, &cast->rhs, tab);
	folder_node(fold, &cast->lhs, tab);

	if (folder_const(cast->rhs))
	{
		Ast_Const *rhs = CAST_CONST(cast->rhs);

		if (folder_const(cast->lhs))
		{
			Ast_Const *lhs = CAST_CONST(cast->lhs);
			bool cond;

			switch (cast->op)
			{
				case TOK_PLUS:
					rhs->value.c_int += lhs->value.c_int;
					*node = (Ast_Node *)rhs;
					break;

				case TOK_MINUS:
					rhs->value.c_int -= lhs->value.c_int;
					*node = (Ast_Node *)rhs;
					break;

				case TOK_STAR:
					rhs->value.c_int *= lhs->value.c_int;
					*node = (Ast_Node *)rhs;
					break;

				case TOK_SLASH:
					// NOTE: Warn on division by zero
					if (MAC_LIKELY(lhs->value.c_int != 0))
					{
						rhs->value.c_int /= lhs->value.c_int;
						*node = (Ast_Node *)rhs;
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant division by zero"));
					}
					break;

				case TOK_PERC:
					// NOTE: Warn on modulo by zero
					if (MAC_LIKELY(lhs->value.c_int != 0))
					{
						rhs->value.c_int %= lhs->value.c_int;
						*node = (Ast_Node *)rhs;
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant module by zero"));
					}
					break;

				case TOK_PLUSF:
					rhs->value.c_float += lhs->value.c_float;
					*node = (Ast_Node *)rhs;
					break;

				case TOK_MINUSF:
					rhs->value.c_float -= lhs->value.c_float;
					*node = (Ast_Node *)rhs;
					break;

				case TOK_STARF:
					rhs->value.c_float *= lhs->value.c_float;
					*node = (Ast_Node *)rhs;
					break;

				case TOK_SLASHF:
					// NOTE: Warn on division by zero
					if (MAC_LIKELY(lhs->value.c_float != 0))
					{
						rhs->value.c_float /= lhs->value.c_float;
						*node = (Ast_Node *)rhs;
					}
					else
					{
						folder_warn(fold, cast->node.loc, STR_STATIC("Constant division by zero"));
					}
					break;

				case TOK_EQ:
					cond = const_equal(CAST_CONST(cast->rhs)->value, CAST_CONST(cast->lhs)->value);
					*node = ast_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				case TOK_NE:
					cond = !const_equal(CAST_CONST(cast->rhs)->value, CAST_CONST(cast->lhs)->value);
					*node = ast_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				case TOK_AMPAMP:
					cond = rhs->value.c_bool && lhs->value.c_bool;
					*node = ast_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
					folder_type(fold, *node, TYPE_BOOL);
					break;

				case TOK_BARBAR:
					cond = rhs->value.c_bool || lhs->value.c_bool;
					*node = ast_const_new(fold->arena, cast->node.loc, const_new(CONST_BOOL, &cond));
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
		Ast_Const *lhs = CAST_CONST(cast->lhs);

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
	else if (cast->rhs->kind == AST_IDENT && cast->lhs->kind == AST_IDENT)
	{
		if (cast->op == TOK_MINUS)
		{
			if (STR_EQUAL(CAST_IDENT(cast->rhs)->name, CAST_IDENT(cast->lhs)->name))
			{
				int32_t zero = 0;
				*node = ast_const_new(fold->arena, cast->node.loc, const_new(CONST_INT, &zero));
				folder_type(fold, *node, TYPE_INT);
			}
		}
		else if (cast->op == TOK_MINUSF)
		{
			if (STR_EQUAL(CAST_IDENT(cast->rhs)->name, CAST_IDENT(cast->lhs)->name))
			{
				double zero = 0;
				*node = ast_const_new(fold->arena, cast->node.loc, const_new(CONST_FLOAT, &zero));
				folder_type(fold, *node, TYPE_FLOAT);
			}
		}
	}
}

static MAC_HOT void
folder_tuple(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	Ast_Tuple *cast = (Ast_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_node(fold, &cast->items[i], tab);
	}
}

static MAC_HOT void
folder_node(Folder *fold, Ast_Node **node, Ast_Tab *tab)
{
	switch ((*node)->kind)
	{
		case AST_IF:
			folder_if(fold, node, tab);
			break;

		case AST_MATCH:
			folder_match(fold, node, tab);
			break;

		case AST_LET:
			folder_let(fold, node, tab);
			break;

		case AST_FUN:
			folder_fun(fold, node, tab);
			break;

		case AST_APP:
			folder_app(fold, node, tab);
			break;

		case AST_IDENT:
			folder_ident(fold, node, tab);
			break;

		case AST_UNARY:
			folder_unary(fold, node, tab);
			break;

		case AST_BINARY:
			folder_binary(fold, node, tab);
			break;

		case AST_TUPLE:
			folder_tuple(fold, node, tab);
			break;

		case AST_CONST:
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
	Ast_Tab tab;
	ast_tab_init(&tab, fold->seed);

	fold->err = err;
	for (size_t i = 0; i < ast->len; ++i)
	{
		if (MAC_LIKELY(ast->items[i]->kind != AST_CONST))
		{
			folder_node(fold, &ast->items[i], &tab);

			if (MAC_UNLIKELY(ast->items[i] == NULL))
			{
				if (i != ast->len)
				{
					memmove(&ast->items[i], &ast->items[i + 1],
							(ast->len - i) * sizeof(Ast_Node *));
					--i;
				}
				--ast->len;
			}
		}
	}
	fold->err = NULL;
}

MAC_HOT void
folder_ast_node(Folder *fold, Ast_Node **node, Error_List *err)
{
	Ast_Tab tab;
	ast_tab_init(&tab, fold->seed);

	fold->err = err;
	folder_node(fold, node, &tab);
	fold->err = NULL;
}
