#include "nml_inlin.h"
#include "nml_expr.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_set.h"
#include "nml_type.h"
#include <stddef.h>

static MAC_HOT void inliner_expr(Inliner *inlin, Expr_Node **node);

static MAC_HOT void
inliner_expr_if(Inliner *inlin, Expr_Node **node)
{
	Expr_If *cast = (Expr_If *)*node;

	inliner_expr(inlin, &cast->cond);
	inliner_expr(inlin, &cast->b_then);
	inliner_expr(inlin, &cast->b_else);
}

static MAC_HOT void
inliner_expr_match(Inliner *inlin, Expr_Node **node)
{
	Expr_Match *cast = (Expr_Match *)*node;

	inliner_expr(inlin, &cast->value);
	for (size_t i = 0; i < cast->len; ++i)
	{
		inliner_expr(inlin, &cast->arms[i].expr);
	}
}

static MAC_HOT void
inliner_expr_let(Inliner *inlin, Expr_Node **node)
{
	Expr_Let *cast = (Expr_Let *)*node;

	inliner_expr(inlin, &cast->value);
	inliner_expr(inlin, &cast->expr);
}

static MAC_HOT void
inliner_expr_fun(Inliner *inlin, Expr_Node **node)
{
	Expr_Fun *cast = (Expr_Fun *)*node;

	inliner_expr(inlin, &cast->body);
	inliner_expr(inlin, &cast->expr);

	// TODO: Named function inlining
	// Desugar function to lambdas
}

static MAC_HOT void
inliner_expr_lambda(Inliner *inlin, Expr_Node **node)
{
	Expr_Lambda *cast = (Expr_Lambda *)*node;

	inliner_expr(inlin, &cast->body);
}

static MAC_HOT void
inliner_expr_app(Inliner *inlin, Expr_Node **node)
{
	Expr_App *cast = (Expr_App *)*node;

	inliner_expr(inlin, &cast->fun);
	for (size_t i = 0; i < cast->len; ++i)
	{
		inliner_expr(inlin, &cast->args[i]);
	}

	// TODO: Check for corner cases
	if (cast->fun->kind == EXPR_LAMBDA)
	{
		Expr_Lambda *fun = (Expr_Lambda *)cast->fun;
		size_t len = MAC_MIN(cast->len, fun->len);

		if (len == fun->len)
		{
			*node = fun->body;
			for (size_t i = len; i > 0; --i)
			{
				*node = expr_let_new(inlin->arena, fun->node.loc, TYPE_NONE,
										fun->pars[i - 1], cast->args[i - 1], *node);
				(*node)->type = cast->args[i - 1]->type;
			}
		}
		else
		{
			for (size_t i = len; i < fun->len; ++i)
			{
				str_set_insert(&inlin->set, fun->pars[i]);
			}

			for (size_t i = len; i > 0; --i)
			{
				if (!str_set_member(&inlin->set, fun->pars[i - 1]))
				{
					fun->body = expr_let_new(inlin->arena, fun->node.loc, TYPE_NONE,
											fun->pars[i - 1], cast->args[i - 1], fun->body);
					fun->body->type = cast->args[i - 1]->type;
				}

				// NOTE: Adjust lambda type
				fun->node.type = MAC_CAST(Type_Fun *, TYPE_PTR(fun->node.type))->ret;
			}
			str_set_reset(&inlin->set);

			fun->len -= len;
			memmove(fun->pars, fun->pars + len, fun->len * sizeof(Sized_Str));

			if (len != cast->len)
			{
				cast->len -= len;
				memmove(cast->args, cast->args + len, cast->len * sizeof(Expr_Node *));
			}
			else
			{
				*node = cast->fun;
			}
		}
	}
}

static MAC_HOT void
inliner_expr_tuple(Inliner *inlin, Expr_Node **node)
{
	Expr_Tuple *cast = (Expr_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		inliner_expr(inlin, &cast->items[i]);
	}
}

static MAC_HOT void
inliner_expr(Inliner *inlin, Expr_Node **node)
{
	switch ((*node)->kind)
	{
		case EXPR_IF:
			inliner_expr_if(inlin, node);
			break;

		case EXPR_MATCH:
			inliner_expr_match(inlin, node);
			break;

		case EXPR_LET:
			inliner_expr_let(inlin, node);
			break;

		case EXPR_FUN:
			inliner_expr_fun(inlin, node);
			break;

		case EXPR_LAMBDA:
			inliner_expr_lambda(inlin, node);
			break;

		case EXPR_APP:
			inliner_expr_app(inlin, node);
			break;

		case EXPR_IDENT:
			break;

		case EXPR_TUPLE:
			inliner_expr_tuple(inlin, node);
			break;

		case EXPR_LIT:
			break;
	}
}

static MAC_HOT void
inliner_decl(Inliner *inlin, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			inliner_expr(inlin, &MAC_CAST(Decl_Let *, node)->value);
			break;

		case DECL_FUN:
			inliner_expr(inlin, &MAC_CAST(Decl_Fun *, node)->body);
			break;

		case DECL_DATA:
			break;

		case DECL_TYPE:
			break;
	}
}

void
inliner_init(Inliner *inlin, Arena *arena, Hash_Default seed)
{
	inlin->arena = arena;
	str_set_init(&inlin->set, seed);
}

void
inliner_free(Inliner *inlin)
{
	str_set_free(&inlin->set);
}

MAC_HOT void
inliner_pass(Inliner *inlin, Ast_Top *ast)
{
	for (size_t i = 0; i < ast->len; ++i)
	{
		inliner_decl(inlin, ast->decls[i]);
	}
}
