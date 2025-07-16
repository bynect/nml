#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "nml_ast.h"
#include "nml_err.h"
#include "nml_fold.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"

static MAC_HOT void folder_node(Folder *fold, Ast_Node **node, Fold_Tab *tab);

static MAC_INLINE inline void
folder_warn(Folder *fold, Sized_Str msg)
{
	error_append(fold->err, fold->arena, msg, 0, ERR_WARN);
}

static MAC_INLINE inline bool
folder_const(Ast_Node *node)
{
	return node->kind == AST_CONST;
}

static MAC_INLINE inline bool
folder_const_equal(Ast_Node *n1, Ast_Node *n2)
{
	Ast_Const *c1 = FOLD_CONST(n1);
	Ast_Const *c2 = FOLD_CONST(n2);

	if (folder_const(n1) && folder_const(n2) &&	c1->kind == c2->kind)
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

static MAC_INLINE inline Ast_Node *
folder_const_copy(Folder *fold, Ast_Const *node)
{
	Ast_Const *tmp = arena_alloc(fold->arena, sizeof(Ast_Const));
	memcpy(tmp, node, sizeof(Ast_Const));
	return (Ast_Node *)tmp;
}

static MAC_HOT void
folder_if(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_If *cast = (Ast_If *)*node;

	folder_node(fold, &cast->cond, tab);
	folder_node(fold, &cast->then, tab);
	if (cast->m_else != NULL)
	{
		folder_node(fold, &cast->m_else, tab);
	}

	if (folder_const(cast->cond) && FOLD_CONST(cast->cond)->kind == CONST_BOOL)
	{
		if (FOLD_CONST(cast->cond)->value.c_bool)
		{
			*node = cast->then;
		}
		else if (!FOLD_CONST(cast->cond)->value.c_bool)
		{
			*node = cast->m_else != NULL ? cast->m_else :
					ast_const_new(fold->arena, CONST_UNIT, 0);
		}
	}
	else if (folder_const(cast->then))
	{
		if (cast->m_else == NULL)
		{
			if (FOLD_CONST(cast->then)->kind == CONST_UNIT)
			{
				*node = cast->then;
			}
		}
		else if (folder_const(cast->m_else) && folder_const_equal(cast->then, cast->m_else))
		{
			*node = cast->then;
		}
	}
}

static MAC_HOT void
folder_match(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_Match *cast = (Ast_Match *)*node;

	folder_node(fold, &cast->value, tab);
	if (cast->arms[0][0]->kind == AST_IDENT)
	{
		// NOTE: If the first pattern is a wildcard directly substitute it
		if (folder_const(cast->value) || cast->value->kind == AST_IDENT)
		{
			Ast_Ident *ident = (Ast_Ident *)cast->arms[0][0];
			if (!STR_EQUAL(ident->name, STR_STATIC("_")))
			{
				Fold_Node value;
				bool shad = fold_tab_get(tab, ident->name, &value);
				fold_tab_set(tab, fold->arena, ident->name, FOLD_NODE(cast->value));

				folder_node(fold, &cast->arms[0][1], tab);
				if (shad)
				{
					fold_tab_set(tab, fold->arena, ident->name, value);
				}
				else
				{
					fold_tab_del(tab, ident->name);
				}
			}
			*node = cast->arms[0][1];
		}
		else
		{
			// TODO: Check for side effects
			*node = ast_let_new(fold->arena, ((Ast_Ident *)cast->arms[0][0])->name, TYPE_NONE,
								cast->value, cast->arms[0][1]);
			folder_node(fold, node, tab);
		}

		folder_warn(fold, STR_STATIC("First pattern in match is a wildcard"));
		return;
	}
	else if (folder_const(cast->value))
	{
		for (size_t i = 0; i < cast->len; ++i)
		{
			if (folder_const(cast->arms[i][0]) &&
				folder_const_equal(cast->value, cast->arms[i][0]))
			{
				*node = cast->arms[i][1];
				return;
			}
			else if (cast->arms[i][0]->kind == AST_IDENT)
			{
				Ast_Ident *ident = (Ast_Ident *)cast->arms[i][0];
				if (!STR_EQUAL(ident->name, STR_STATIC("_")))
				{
					Fold_Node value;
					bool shad = fold_tab_get(tab, ident->name, &value);
					fold_tab_set(tab, fold->arena, ident->name, FOLD_NODE(cast->value));

					folder_node(fold, &cast->arms[i][1], tab);
					if (shad)
					{
						fold_tab_set(tab, fold->arena, ident->name, value);
					}
					else
					{
						fold_tab_del(tab, ident->name);
					}
				}
				*node = cast->arms[i][1];
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
			consts[i] = folder_const(tuple->items[i]);
		}

		Fold_Tab tmp;
		fold_tab_init(&tmp, tab->seed);

		bool match = true;
		for (size_t i = 0; i < cast->len; ++i)
		{
			fold_tab_merge(tab, &tmp, fold->arena);
			if (MAC_LIKELY(cast->arms[i][0]->kind == AST_TUPLE))
			{
				Ast_Tuple *patn = (Ast_Tuple *)cast->arms[i][0];
				for (size_t j = 0; j < tuple->len; ++j)
				{
					if (folder_const(patn->items[j]))
					{
						match = consts[j] && folder_const_equal(patn->items[j], tuple->items[j]);
					}
					else if (patn->items[j]->kind == AST_IDENT)
					{
						Ast_Ident *ident = (Ast_Ident *)patn->items[j];

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
								fold_tab_set(&tmp, fold->arena, ident->name, FOLD_NODE(tuple->items[j]));
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
					folder_node(fold, &cast->arms[i][1], &tmp);
					*node = cast->arms[i][1];
					return;
				}
			}
			fold_tab_reset(&tmp);
		}
	}

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_node(fold, &cast->arms[i][1], tab);

		// NOTE: Remove unreachable patterns
		if (MAC_UNLIKELY(cast->arms[i][0]->kind == AST_IDENT))
		{
			cast->len = i;
			break;
		}
	}

	// TODO: Improve pattern matching folding
	// Notably add support for nested constant tuple.
}

static MAC_HOT void
folder_let(Folder *fold, Ast_Node **node, Fold_Tab *tab)
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
	else if (folder_const(cast->value))
	{
		if (cast->expr != NULL)
		{
			Fold_Node value;
			bool shad = fold_tab_get(tab, cast->name, &value);
			fold_tab_set(tab, fold->arena, cast->name, FOLD_NODE(cast->value));

			folder_node(fold, &cast->expr, tab);
			*node = cast->expr;

			if (shad)
			{
				fold_tab_set(tab, fold->arena, cast->name, value);
			}
			else
			{
				fold_tab_del(tab, cast->name);
			}
		}
		else
		{
			fold_tab_set(tab, fold->arena, cast->name, FOLD_NODE(cast->value));
		}
	}
	else if (cast->expr != NULL)
	{
		Fold_Node value;
		bool shad = fold_tab_get(tab, cast->name, &value);
		fold_tab_set(tab, fold->arena, cast->name, FOLD_NODE(cast->value));

		folder_node(fold, &cast->expr, tab);
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
			fold_tab_set(tab, fold->arena, cast->name, value);
		}
		else
		{
			fold_tab_del(tab, cast->name);
		}
	}
}

static MAC_HOT void
folder_fun(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_Fun *cast = (Ast_Fun *)*node;

	folder_node(fold, &cast->body, tab);
	if (cast->expr != NULL)
	{
		folder_node(fold, &cast->expr, tab);
	}
}

static MAC_HOT void
folder_app(Folder *fold, Ast_Node **node, Fold_Tab *tab)
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
}

static MAC_HOT void
folder_ident(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_Ident *cast = (Ast_Ident *)*node;

	Fold_Node value;
	if (fold_tab_get(tab, cast->name, &value))
	{
		if (folder_const(value.node))
		{
			*node = folder_const_copy(fold, FOLD_CONST(value.node));
		}
		else if (value.node->kind == AST_IDENT)
		{
			cast->name = ((Ast_Ident *)value.node)->name;
		}
		else
		{
			// NOTE: Check for side effects and code duplication
			*node = value.node;
			folder_warn(fold, STR_STATIC("Substituted value is not constant"));
		}

		if (MAC_UNLIKELY(value.dead))
		{
			value.dead = false;
			fold_tab_set(tab, fold->arena, cast->name, value);
		}
	}
}

static MAC_HOT void
folder_unary(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_Unary *cast = (Ast_Unary *)*node;

	folder_node(fold, &cast->lhs, tab);
	if (folder_const(cast->lhs))
	{
		Ast_Const *lhs = FOLD_CONST(cast->lhs);
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
folder_binary(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_Binary *cast = (Ast_Binary *)*node;

	folder_node(fold, &cast->rhs, tab);
	folder_node(fold, &cast->lhs, tab);

	if (folder_const(cast->rhs))
	{
		Ast_Const *rhs = FOLD_CONST(cast->rhs);

		if (folder_const(cast->lhs))
		{
			Ast_Const *lhs = FOLD_CONST(cast->lhs);
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
					if (lhs->value.c_int != 0)
					{
						rhs->value.c_int /= lhs->value.c_int;
						*node = (Ast_Node *)rhs;
					}
					else
					{
						folder_warn(fold, STR_STATIC("Constant division by zero"));
					}
					break;

				case TOK_PERC:
					// NOTE: Warn on modulo by zero
					if (lhs->value.c_int != 0)
					{
						rhs->value.c_int %= lhs->value.c_int;
						*node = (Ast_Node *)rhs;
					}
					else
					{
						folder_warn(fold, STR_STATIC("Constant modulo by zero"));
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
					if (lhs->value.c_float != 0)
					{
						rhs->value.c_float /= lhs->value.c_float;
						*node = (Ast_Node *)rhs;
					}
					else
					{
						folder_warn(fold, STR_STATIC("Constant division by zero"));
					}
					break;

				case TOK_EQ:
					cond = folder_const_equal(cast->rhs, cast->lhs);
					*node = ast_const_new(fold->arena, CONST_BOOL, &cond);
					break;

				case TOK_NE:
					cond = !folder_const_equal(cast->rhs, cast->lhs);
					*node = ast_const_new(fold->arena, CONST_BOOL, &cond);
					break;

				case TOK_AMPAMP:
					cond = rhs->value.c_bool && lhs->value.c_bool;
					*node = ast_const_new(fold->arena, CONST_BOOL, &cond);
					break;

				case TOK_BARBAR:
					cond = rhs->value.c_bool || lhs->value.c_bool;
					*node = ast_const_new(fold->arena, CONST_BOOL, &cond);
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
	else if (folder_const(cast->lhs))
	{
		Ast_Const *lhs = FOLD_CONST(cast->lhs);

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
	else if (cast->rhs->kind == AST_IDENT && cast->lhs->kind == AST_IDENT)
	{
		if (STR_EQUAL(((Ast_Ident *)cast->rhs)->name, ((Ast_Ident *)cast->lhs)->name))
		{
			int32_t zero = 0;
			double zerof = 0;

			switch (cast->op)
			{
				case TOK_MINUS:
					*node = ast_const_new(fold->arena, CONST_INT, &zero);
					break;

				case TOK_MINUSF:
					*node = ast_const_new(fold->arena, CONST_FLOAT, &zerof);
					break;

				default:
					break;
			}
		}
	}
}

static MAC_HOT void
folder_tuple(Folder *fold, Ast_Node **node, Fold_Tab *tab)
{
	Ast_Tuple *cast = (Ast_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		folder_node(fold, &cast->items[i], tab);
	}
}

static MAC_HOT void
folder_node(Folder *fold, Ast_Node **node, Fold_Tab *tab)
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
	Fold_Tab tab;
	fold_tab_init(&tab, fold->seed);

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
	Fold_Tab tab;
	fold_tab_init(&tab, fold->seed);

	fold->err = err;
	folder_node(fold, node, &tab);
	fold->err = NULL;
}
