#include "nml_hoist.h"
#include "nml_decl.h"
#include "nml_expr.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_fmt.h"
#include "nml_str.h"
#include "nml_type.h"

static MAC_HOT void hoister_expr(Hoister *hoist, Expr_Node *node);

static MAC_INLINE inline void
hoister_append(Hoister *hoist, Decl_Node *node)
{
	if (hoist->len + 1 >= hoist->size)
	{
		hoist->size += 12;
		hoist->decls = arena_update(hoist->arena, hoist->decls,
									hoist->size * sizeof(Decl_Node *));
	}
	hoist->decls[hoist->len++] = node;
}

static MAC_HOT void
hoister_if(Hoister *hoist, Expr_If *node)
{
	hoister_expr(hoist, node->cond);
	hoister_expr(hoist, node->b_then);
	hoister_expr(hoist, node->b_else);
}

static MAC_HOT void
hoister_match(Hoister *hoist, Expr_Match *node)
{
	hoister_expr(hoist, node->value);
	for (size_t i = 0; i < node->len; ++i)
	{
		hoister_expr(hoist, node->arms[i].expr);
	}
}

static MAC_HOT void
hoister_let(Hoister *hoist, Expr_Let *node)
{
	hoister_expr(hoist, node->value);
	hoister_expr(hoist, node->expr);
}

static MAC_HOT void
hoister_fun(Hoister *hoist, Expr_Fun *node)
{
	hoister_expr(hoist, node->body);
	hoister_expr(hoist, node->expr);

	Decl_Node *decl = decl_fun_new(hoist->arena, node->node.loc, TYPE_NONE,
									node->name, node->pars, node->len, node->body);

	node->label = hoist->label++;
	MAC_CAST(Decl_Fun *, decl)->label = node->label;
	hoister_append(hoist, decl);
}

static MAC_HOT void
hoister_lambda(Hoister *hoist, Expr_Lambda *node)
{
	hoister_expr(hoist, node->body);

	Decl_Node *decl = decl_fun_new(hoist->arena, node->node.loc, TYPE_NONE,
									STR_STATIC("@lambda"), node->pars, node->len, node->body);

	node->label = hoist->label++;
	MAC_CAST(Decl_Fun *, decl)->label = node->label;
	hoister_append(hoist, decl);
}

static MAC_HOT void
hoister_app(Hoister *hoist, Expr_App *node)
{
	hoister_expr(hoist, node->fun);
	for (size_t i = 0; i < node->len; ++i)
	{
		hoister_expr(hoist, node->args[i]);
	}
}

static MAC_HOT void
hoister_tuple(Hoister *hoist, Expr_Tuple *node)
{
	for (size_t i = 0; i < node->len; ++i)
	{
		hoister_expr(hoist, node->items[i]);
	}
}

static MAC_HOT void
hoister_expr(Hoister *hoist, Expr_Node *node)
{
	switch (node->kind)
	{
		case EXPR_IF:
			hoister_if(hoist, (Expr_If *)node);
			break;

		case EXPR_MATCH:
			hoister_match(hoist, (Expr_Match *)node);
			break;

		case EXPR_LET:
			hoister_let(hoist, (Expr_Let *)node);
			break;

		case EXPR_FUN:
			hoister_fun(hoist, (Expr_Fun *)node);
			break;

		case EXPR_LAMBDA:
			hoister_lambda(hoist, (Expr_Lambda *)node);
			break;

		case EXPR_APP:
			hoister_app(hoist, (Expr_App *)node);
			break;

		case EXPR_IDENT:
			break;

		case EXPR_TUPLE:
			hoister_tuple(hoist, (Expr_Tuple *)node);
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_HOT void
hoister_decl(Hoister *hoist, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			if (MAC_CAST(Decl_Let *, node)->value->kind == EXPR_LAMBDA)
			{
				Expr_Lambda *fun = (Expr_Lambda *)MAC_CAST(Decl_Let *, node)->value;
				node = decl_fun_new(hoist->arena, node->loc, MAC_CAST(Decl_Let *, node)->hint,
									MAC_CAST(Decl_Let *, node)->name, fun->pars, fun->len, fun->body);
			}
			else
			{
				hoister_expr(hoist, MAC_CAST(Decl_Let *, node)->value);
			}
			break;

		case DECL_FUN:
			hoister_expr(hoist, MAC_CAST(Decl_Fun *, node)->body);
			break;

		case DECL_DATA:
			break;

		case DECL_TYPE:
			break;
	}
	hoister_append(hoist, node);
}

void
hoister_init(Hoister *hoist, Arena *arena)
{
	hoist->arena = arena;
	hoist->label = 0;

	hoist->decls = NULL;
	hoist->len = 0;
	hoist->size = 0;
}

MAC_HOT void
hoister_pass(Hoister *hoist, Ast_Top *ast)
{
	Decl_Node *tmp[ast->len];
	memcpy(tmp, ast->decls, ast->len * sizeof(Decl_Node *));

	hoist->decls = ast->decls;
	hoist->len = 0;
	hoist->size = ast->len;

	hoist->label = 1;
	for (size_t i = 0; i < ast->len; ++i)
	{
		hoister_decl(hoist, tmp[i]);
	}

	ast->decls = hoist->decls;
	ast->len = hoist->len;
}
