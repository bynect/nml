#include "nml_merge.h"
#include "nml_expr.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include <stddef.h>
#include <string.h>

static MAC_HOT void merger_expr(Merger *merge, Expr_Node *node);

static MAC_HOT void
merger_if(Merger *merge, Expr_If *node)
{
	merger_expr(merge, node->cond);
	merger_expr(merge, node->b_then);
	merger_expr(merge, node->b_else);
}

static MAC_HOT void
merger_match(Merger *merge, Expr_Match *node)
{
	merger_expr(merge, node->value);
	for (size_t i = 0; i < node->len; ++i)
	{
		merger_expr(merge, node->arms[i].expr);
	}
}

static MAC_HOT void
merger_let(Merger *merge, Expr_Let *node)
{
	merger_expr(merge, node->value);
	merger_expr(merge, node->expr);
}

static MAC_HOT void
merger_fun(Merger *merge, Expr_Fun *node)
{
	merger_expr(merge, node->body);
	merger_expr(merge, node->expr);
}

static MAC_HOT void
merger_lambda(Merger *merge, Expr_Lambda *node)
{
	bool lambda_chain = merge->lambda_chain;
	if (node->body->kind == EXPR_LAMBDA)
	{
		if (merge->lambda_chain)
		{
			merger_expr(merge, node->body);
		}
		else
		{
			merge->lambda_chain = true;
			merger_expr(merge, node->body);

			Expr_Lambda *body = (Expr_Lambda *)node->body;
			size_t len = node->len;

			while (body->node.kind == EXPR_LAMBDA)
			{
				len += body->len;
				body = (Expr_Lambda *)body->body;
			}

			Sized_Str *pars = arena_alloc(merge->arena, len * sizeof(Sized_Str));
			size_t off = 0;

			body = (Expr_Lambda *)node;
			while (body->node.kind == EXPR_LAMBDA)
			{
				memcpy(pars + off, body->pars, body->len * sizeof(Sized_Str));
				off += body->len;
				body = (Expr_Lambda *)body->body;
			}

			node->pars = pars;
			node->len = len;
			node->body = (Expr_Node *)body;
		}
	}
	else if (merge->lambda_chain)
	{
		merge->lambda_chain = false;
		merger_expr(merge, node->body);
	}
	merge->lambda_chain = lambda_chain;
}

static MAC_HOT void
merger_app(Merger *merge, Expr_App *node)
{
	merger_expr(merge, node->fun);
	for (size_t i = 0; i < node->len; ++i)
	{
		merger_expr(merge, node->args[i]);
	}
}

static MAC_HOT void
merger_tuple(Merger *merge, Expr_Tuple *node)
{
	for (size_t i = 0; i < node->len; ++i)
	{
		merger_expr(merge, node->items[i]);
	}
}

static MAC_HOT void
merger_expr(Merger *merge, Expr_Node *node)
{
	switch (node->kind)
	{
		case EXPR_IF:
			merger_if(merge, (Expr_If *)node);
			break;

		case EXPR_MATCH:
			merger_match(merge, (Expr_Match *)node);
			break;

		case EXPR_LET:
			merger_let(merge, (Expr_Let *)node);
			break;

		case EXPR_FUN:
			merger_fun(merge, (Expr_Fun *)node);
			break;

		case EXPR_LAMBDA:
			merger_lambda(merge, (Expr_Lambda *)node);
			break;

		case EXPR_APP:
			merger_app(merge, (Expr_App *)node);
			break;

		case EXPR_IDENT:
			break;

		case EXPR_TUPLE:
			merger_tuple(merge, (Expr_Tuple *)node);
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_HOT void
merger_decl(Merger *merge, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			merger_expr(merge, MAC_CAST(Decl_Let *, node)->value);
			break;

		case DECL_FUN:
			merger_expr(merge, MAC_CAST(Decl_Fun *, node)->body);
			break;

		case DECL_DATA:
			break;

		case DECL_TYPE:
			break;
	}
}

void
merger_init(Merger *merge, Arena *arena)
{
	merge->arena = arena;
	merge->lambda_chain = false;
	merge->app_chain = false;
}

MAC_HOT void
merger_pass(Merger *merge, Ast_Top *ast)
{
	for (size_t i = 0; i < ast->len; ++i)
	{
		merger_decl(merge, ast->decls[i]);
	}
}
