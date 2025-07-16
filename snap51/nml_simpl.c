#include "nml_simpl.h"
#include "nml_decl.h"
#include "nml_expr.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_fmt.h"

static MAC_INLINE inline void simplifier_expr(Simplifier *simpl, Expr_Node **node);

static MAC_HOT void
simplifier_push(Simplifier *simpl, Decl_Node *node)
{
	if (MAC_UNLIKELY(simpl->len + 1 == simpl->size))
	{
		simpl->size += 16;
		simpl->decls = arena_update(simpl->arena, simpl->decls, simpl->size * sizeof(Decl_Node *));
	}

	simpl->decls[simpl->len++] = node;
}

static MAC_HOT void
simplifier_expr_if(Simplifier *simpl, Expr_Node **node)
{
	Expr_If *cast = (Expr_If *)*node;

	simplifier_expr(simpl, &cast->cond);
	simplifier_expr(simpl, &cast->b_then);
	simplifier_expr(simpl, &cast->b_else);
}

static MAC_HOT void
simplifier_expr_match(Simplifier *simpl, Expr_Node **node)
{
	Expr_Match *cast = (Expr_Match *)*node;

	simplifier_expr(simpl, &cast->value);
	for (size_t i = 0; i < cast->len; ++i)
	{
		simplifier_expr(simpl, &cast->arms[i].expr);
	}
}

static MAC_HOT void
simplifier_expr_let(Simplifier *simpl, Expr_Node **node)
{
	Expr_Let *cast = (Expr_Let *)*node;

	simplifier_expr(simpl, &cast->value);
	simplifier_expr(simpl, &cast->expr);
}

static MAC_HOT void
simplifier_expr_fun(Simplifier *simpl, Expr_Node **node)
{
	Expr_Fun *cast = (Expr_Fun *)*node;

	simplifier_expr(simpl, &cast->body);
	simplifier_expr(simpl, &cast->expr);

	Decl_Node *decl = decl_fun_new(simpl->arena, cast->node.loc, cast->hint, cast->name, cast->pars, cast->len, cast->body);
	MAC_CAST(Decl_Fun *, decl)->fun = cast->fun;
	simplifier_push(simpl, decl);
}

static MAC_HOT void
simplifier_expr_lambda(Simplifier *simpl, Expr_Node **node)
{
	Expr_Lambda *cast = (Expr_Lambda *)*node;

	simplifier_expr(simpl, &cast->body);

	Sized_Str name = format_str(simpl->arena, STR_STATIC("@lambda@{u}"), 1);
	//*node = expr_ident_new(simpl->arena, cast->node.loc, name);

	Decl_Node *decl = decl_fun_new(simpl->arena, cast->node.loc, TYPE_NONE, name, cast->pars, cast->len, cast->body);
	MAC_CAST(Decl_Fun *, decl)->fun = cast->node.type;
	simplifier_push(simpl, decl);
}

static MAC_HOT void
simplifier_expr_app(Simplifier *simpl, Expr_Node **node)
{
	Expr_App *cast = (Expr_App *)*node;

	simplifier_expr(simpl, &cast->fun);
	for (size_t i = 0; i < cast->len; ++i)
	{
		simplifier_expr(simpl, &cast->args[i]);
	}
}

static MAC_HOT void
simplifier_expr_ident(Simplifier *simpl, Expr_Node **node)
{
	Expr_Ident *cast = (Expr_Ident *)*node;
}

static MAC_HOT void
simplifier_expr_tuple(Simplifier *simpl, Expr_Node **node)
{
	Expr_Tuple *cast = (Expr_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		simplifier_expr(simpl, &cast->items[i]);
	}
}

static MAC_HOT void
simplifier_expr(Simplifier *simpl, Expr_Node **node)
{
	switch ((*node)->kind)
	{
		case EXPR_IF:
			simplifier_expr_if(simpl, node);
			break;

		case EXPR_MATCH:
			simplifier_expr_match(simpl, node);
			break;

		case EXPR_LET:
			simplifier_expr_let(simpl, node);
			break;

		case EXPR_FUN:
			simplifier_expr_fun(simpl, node);
			break;

		case EXPR_LAMBDA:
			simplifier_expr_lambda(simpl, node);
			break;

		case EXPR_APP:
			simplifier_expr_app(simpl, node);
			break;

		case EXPR_IDENT:
			simplifier_expr_ident(simpl, node);
			break;

		case EXPR_TUPLE:
			simplifier_expr_tuple(simpl, node);
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_INLINE inline void
simplifier_decl(Simplifier *simpl, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			simplifier_expr(simpl, &MAC_CAST(Decl_Let *, node)->value);
			simplifier_push(simpl, node);
			break;

		case DECL_FUN:
			simplifier_expr(simpl, &MAC_CAST(Decl_Fun *, node)->body);
			simplifier_push(simpl, node);
			break;

		case DECL_DATA:
			// TODO
			break;

		case DECL_TYPE:
			// TODO
			break;
	}
}

void
simplifier_init(Simplifier *simpl, Arena *arena)
{
	simpl->arena = arena;
	simpl->decls = NULL;
	simpl->len = 0;
	simpl->size = 0;
}

MAC_HOT void
simplifier_simplify(Simplifier *simpl, Ast_Top *ast)
{
	simpl->size = ast->len;
	simpl->decls = arena_lock(simpl->arena, simpl->size * sizeof(Decl_Node *));

	for (size_t i = 0; i < ast->len; ++i)
	{
		simplifier_decl(simpl, ast->decls[i]);
	}

	if (simpl->len == ast->len)
	{
		arena_update(simpl->arena, simpl->decls, 0);
	}
	else
	{
		arena_update(simpl->arena, simpl->decls, simpl->len * sizeof(Decl_Node *));
		arena_unlock(simpl->arena, simpl->decls);

		ast->decls = simpl->decls;
		ast->len = simpl->len;
	}
}
