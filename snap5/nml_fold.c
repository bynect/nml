#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "nml_ast.h"
#include "nml_fold.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"

static MAC_HOT void fold_node(Arena *arena, Expr_Node **node, Fold_Tab *tab);

static MAC_INLINE inline bool
fold_const(Expr_Node *node)
{
	return node->kind == EXPR_CONST;
}

static MAC_INLINE inline Const_Node *
fold_const_cast(Expr_Node *node)
{
	return fold_const(node) ? &((Expr_Const *)node)->cons : NULL;
}

static MAC_INLINE inline bool
fold_const_equal(Expr_Node *n1, Expr_Node *n2)
{
	Const_Node *c1 = fold_const_cast(n1);
	Const_Node *c2 = fold_const_cast(n2);

	if (c1 != NULL && c2 != NULL && c1->kind == c2->kind)
	{
		switch (c1->kind)
		{
			case CONST_UNIT:
				return true;

			case CONST_INT:
				return c1->value.c_int == c2->value.c_int;

			case CONST_FLOAT:
				return c1->value.c_float == c2->value.c_float;

			case CONST_STR:
				return STR_EQUAL(c1->value.c_str, c2->value.c_str);

			case CONST_CHAR:
				return c1->value.c_char == c2->value.c_char;

			case CONST_BOOL:
				return c1->value.c_bool == c2->value.c_bool;
		}
	}
	return false;
}

static MAC_INLINE inline Expr_Node *
fold_const_copy(Arena *arena, Expr_Const *node)
{
	Expr_Const *tmp = arena_alloc(arena, sizeof(Expr_Const));
	memcpy(tmp, node, sizeof(Expr_Const));
	return (Expr_Node *)tmp;
}

static MAC_HOT void
fold_expr_if(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_If *cast = (Expr_If *)*node;

	fold_node(arena, &cast->cond, tab);
	fold_node(arena, &cast->then, tab);
	if (cast->m_else != NULL)
	{
		fold_node(arena, &cast->m_else, tab);
	}

	if (fold_const(cast->cond))
	{
		Const_Node *cond = fold_const_cast(cast->cond);
		if (MAC_LIKELY(cond->kind == CONST_BOOL))
		{
			if (cond->value.c_bool)
			{
				*node = cast->then;
			}
			else
			{
				*node = cast->m_else != NULL ? cast->m_else :
						expr_const_new(arena, CONST_UNIT, 0);
			}
		}
	}
	else if (fold_const(cast->then))
	{
		Const_Node *then = fold_const_cast(cast->cond);
		if (cast->m_else == NULL)
		{
			if (then->kind == CONST_UNIT)
			{
				*node = cast->then;
			}
		}
		else if (fold_const(cast->m_else) && fold_const_equal(cast->then, cast->m_else))
		{
			*node = cast->then;
		}
	}
}

static MAC_HOT void
fold_expr_let(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_Let *cast = (Expr_Let *)*node;

	fold_node(arena, &cast->value, tab);

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
	else if (fold_const(cast->value))
	{
		if (cast->expr != NULL)
		{
			Fold_Node fold;
			bool shad = fold_tab_get(tab, cast->name, &fold);
			fold_tab_set(tab, arena, cast->name, FOLD_NODE(cast->value));

			fold_node(arena, &cast->expr, tab);
			*node = cast->expr;

			if (shad)
			{
				fold_tab_set(tab, arena, cast->name, fold);
			}
			else
			{
				fold_tab_del(tab, cast->name);
			}
		}
		else
		{
			fold_tab_set(tab, arena, cast->name, FOLD_NODE(cast->value));
		}
	}
	else if (cast->expr != NULL)
	{
		Fold_Node fold;
		bool shad = fold_tab_get(tab, cast->name, &fold);
		fold_tab_set(tab, arena, cast->name, FOLD_NODE(cast->value));

		fold_node(arena, &cast->expr, tab);
		Fold_Node dead;
		if (MAC_UNLIKELY(fold_tab_get(tab, cast->name, &dead) && dead.dead))
		{
			// TODO: Check for side effects
			// Eliminating the let expression when the bound value is not used
			// will yield incorrect results in case of side effects.
			//
			// However, since side effects can only be the results of impure function
			// applications, not eliminating if the let bound value contains an impure
			// function application (or in case of function application altogether),
			// will keep correctly the behaviour of the program.
			*node = cast->expr;
		}

		if (shad)
		{
			fold_tab_set(tab, arena, cast->name, fold);
		}
		else
		{
			fold_tab_del(tab, cast->name);
		}
	}
}

static MAC_HOT void
fold_expr_fun(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_Fun *cast = (Expr_Fun *)*node;

	fold_node(arena, &cast->body, tab);
	if (cast->expr != NULL)
	{
		fold_node(arena, &cast->expr, tab);
	}
}

static MAC_HOT void
fold_expr_app(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_App *cast = (Expr_App *)*node;

	fold_node(arena, &cast->fun, tab);
	for (size_t i = 0; i < cast->len; ++i)
	{
		fold_node(arena, &cast->args[i], tab);
	}

	// TODO: Function inlining
	// If the function being applied is made up only of constant
	// operations, function inlining and further folding are possible.
}

static MAC_HOT void
fold_expr_ident(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_Ident *cast = (Expr_Ident *)*node;

	Fold_Node fold;
	if (fold_tab_get(tab, cast->name, &fold))
	{
		if (fold_const(fold.node))
		{
			*node = fold_const_copy(arena, (Expr_Const *)fold.node);
		}

		if (MAC_UNLIKELY(fold.dead))
		{
			fold.dead = false;
			fold_tab_set(tab, arena, cast->name, fold);
		}
	}
}

static MAC_HOT void
fold_expr_unary(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_Unary *cast = (Expr_Unary *)*node;

	fold_node(arena, &cast->lhs, tab);

	if (fold_const(cast->lhs))
	{
		Const_Node *lhs = fold_const_cast(cast->lhs);
		if (cast->op == TOK_MINUS)
		{
			if (lhs->kind == CONST_INT)
			{
				lhs->value.c_int = -lhs->value.c_int;
			}
			else if (lhs->kind == CONST_FLOAT)
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
fold_expr_binary(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_Binary *cast = (Expr_Binary *)*node;

	fold_node(arena, &cast->rhs, tab);
	fold_node(arena, &cast->lhs, tab);

	if (fold_const(cast->rhs))
	{
		Const_Node *rhs = fold_const_cast(cast->rhs);

		if (fold_const(cast->lhs))
		{
			Const_Node *lhs = fold_const_cast(cast->lhs);
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
					// NOTE: Ignore division by zero
					if (lhs->value.c_int != 0)
					{
						rhs->value.c_int /= lhs->value.c_int;
						*node = (Expr_Node *)rhs;
					}
					break;

				case TOK_PERC:
					// NOTE: Ignore modulo by zero
					if (lhs->value.c_int != 0)
					{
						rhs->value.c_int %= lhs->value.c_int;
						*node = (Expr_Node *)rhs;
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
					// NOTE: Ignore division by zero
					if (lhs->value.c_float != 0)
					{
						rhs->value.c_float /= lhs->value.c_float;
						*node = (Expr_Node *)rhs;
					}
					break;

				case TOK_EQ:
					cond = fold_const_equal(cast->rhs, cast->lhs);
					*node = expr_const_new(arena, CONST_BOOL, &cond);
					break;

				case TOK_NE:
					cond = !fold_const_equal(cast->rhs, cast->lhs);
					*node = expr_const_new(arena, CONST_BOOL, &cond);
					break;

				case TOK_AMPAMP:
					cond = rhs->value.c_bool && lhs->value.c_bool;
					*node = expr_const_new(arena, CONST_BOOL, &cond);
					break;

				case TOK_BARBAR:
					cond = rhs->value.c_bool || lhs->value.c_bool;
					*node = expr_const_new(arena, CONST_BOOL, &cond);
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
					if (rhs->value.c_int == 0)
					{
						*node = cast->lhs;
					}
					break;

				case TOK_PLUSF:
				case TOK_MINUSF:
					if (rhs->value.c_float == 0)
					{
						*node = cast->lhs;
					}
					break;

				case TOK_STAR:
					if (rhs->value.c_int == 0)
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
					else if (rhs->value.c_int == 1)
					{
						*node = cast->lhs;
					}
					break;

				case TOK_STARF:
					if (rhs->value.c_float == 0)
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
					else if (rhs->value.c_float == 1)
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
	else if (fold_const(cast->lhs))
	{
		Const_Node *lhs = fold_const_cast(cast->lhs);

		switch (cast->op)
		{
			case TOK_PLUS:
			case TOK_MINUS:
				if (lhs->value.c_int == 0)
				{
					*node = cast->rhs;
				}
				break;

			case TOK_PLUSF:
			case TOK_MINUSF:
				if (lhs->value.c_float == 0)
				{
					*node = cast->rhs;
				}
				break;

			case TOK_STAR:
				if (lhs->value.c_int == 0)
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
				else if (lhs->value.c_int == 1)
				{
					*node = cast->rhs;
				}
				break;

			case TOK_STARF:
				if (lhs->value.c_float == 0)
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
				else if (lhs->value.c_float == 1)
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
		if (STR_EQUAL(((Expr_Ident *)cast->rhs)->name, ((Expr_Ident *)cast->lhs)->name))
		{
			int32_t zero = 0;
			double zerof = 0;

			switch (cast->op)
			{
				case TOK_MINUS:
					*node = expr_const_new(arena, CONST_INT, &zero);
					break;

				case TOK_MINUSF:
					*node = expr_const_new(arena, CONST_FLOAT, &zerof);
					break;

				default:
					break;
			}
		}
	}
}

static MAC_HOT void
fold_expr_tuple(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	Expr_Tuple *cast = (Expr_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		fold_node(arena, &cast->items[i], tab);
	}
}

static MAC_HOT void
fold_node(Arena *arena, Expr_Node **node, Fold_Tab *tab)
{
	switch ((*node)->kind)
	{
		case EXPR_IF:
			fold_expr_if(arena, node, tab);
			break;

		case EXPR_LET:
			fold_expr_let(arena, node, tab);
			break;

		case EXPR_FUN:
			fold_expr_fun(arena, node, tab);
			break;

		case EXPR_APP:
			fold_expr_app(arena, node, tab);
			break;

		case EXPR_IDENT:
			fold_expr_ident(arena, node, tab);
			break;

		case EXPR_UNARY:
			fold_expr_unary(arena, node, tab);
			break;

		case EXPR_BINARY:
			fold_expr_binary(arena, node, tab);
			break;

		case EXPR_TUPLE:
			fold_expr_tuple(arena, node, tab);
			break;

		case EXPR_CONST:
			break;
	}
}

MAC_HOT void
fold_ast(Arena *arena, Ast_Module *ast, Hash_Default seed)
{
	Fold_Tab tab;
	fold_tab_init(&tab, seed);

	for (size_t i = 0; i < ast->len; )
	{
		if (((Expr_Node *)ast->items[i])->kind != EXPR_CONST)
		{
			fold_node(arena, (Expr_Node **)&ast->items[i], &tab);

			if (MAC_UNLIKELY(ast->items[i] == NULL))
			{
				if (i < ast->len)
				{
					memmove(&ast->items[i], &ast->items[i + 1],
							(ast->len - i) * sizeof(Expr_Node *));
				}
				--ast->len;
				continue;
			}
			++i;
		}
	}
}

MAC_HOT void
fold_ast_node(Arena *arena, Expr_Node **node, Hash_Default seed)
{
	Fold_Tab tab;
	fold_tab_init(&tab, seed);
	fold_node(arena, node, &tab);
}
