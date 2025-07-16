#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nml_comp.h"
#include "nml_dbg.h"
#include "nml_decl.h"
#include "nml_fmt.h"
#include "nml_expr.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"

static MAC_HOT void compiler_expr(Compiler *comp, Expr_Node **node);

static MAC_INLINE inline void
compiler_error(Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
}

static MAC_INLINE inline uint32_t
compiler_id(Compiler *comp)
{
	return comp->id++;
}

static MAC_HOT void
compiler_expr_if(Compiler *comp, Expr_Node **node)
{
	Expr_If *cast = (Expr_If *)*node;

	compiler_expr(comp, &cast->cond);
	compiler_expr(comp, &cast->b_then);
	compiler_expr(comp, &cast->b_else);
}

static MAC_HOT void
compiler_expr_match(Compiler *comp, Expr_Node **node)
{
	Expr_Match *cast = (Expr_Match *)*node;

	compiler_expr(comp, &cast->value);
	for (size_t i = 0; i < cast->len; ++i)
	{
		compiler_expr(comp, &cast->arms[i].expr);
	}
}

static MAC_HOT void
compiler_expr_let(Compiler *comp, Expr_Node **node)
{
	Expr_Let *cast = (Expr_Let *)*node;

	compiler_expr(comp, &cast->value);
	compiler_expr(comp, &cast->expr);
}

static MAC_HOT void
compiler_expr_fun(Compiler *comp, Expr_Node **node)
{
	Expr_Fun *cast = (Expr_Fun *)*node;

	compiler_expr(comp, &cast->body);
	compiler_expr(comp, &cast->expr);

	Global glob = {
		.name = format_str(comp->arena, STR_STATIC("{S}@{u}"),
							cast->name, compiler_id(comp)),
		.expr = cast->body,
		.fun = true,
	};
	glob_buf_push(&comp->buf, glob);
}

static MAC_HOT void
compiler_expr_lambda(Compiler *comp, Expr_Node **node)
{
	Expr_Lambda *cast = (Expr_Lambda *)*node;

	compiler_expr(comp, &cast->body);

	Global glob = {
		.name = format_str(comp->arena, STR_STATIC("@lambda@{u}"),
							compiler_id(comp)),
		// TODO: Expr_Lambda to Decl_Fun (ditto for Expr_Fun)
		.expr = *node,
		.fun = true,
	};
	glob_buf_push(&comp->buf, glob);

	*node = expr_ident_new(comp->arena, cast->node.loc, glob.name);
}

static MAC_HOT void
compiler_expr_app(Compiler *comp, Expr_Node **node)
{
	Expr_App *cast = (Expr_App *)*node;

	compiler_expr(comp, &cast->fun);
	for (size_t i = 0; i < cast->len; ++i)
	{
		compiler_expr(comp, &cast->args[i]);
	}
}

static MAC_HOT void
compiler_expr_tuple(Compiler *comp, Expr_Node **node)
{
	Expr_Tuple *cast = (Expr_Tuple *)*node;

	for (size_t i = 0; i < cast->len; ++i)
	{
		compiler_expr(comp, &cast->items[i]);
	}
}

static MAC_HOT void
compiler_expr(Compiler *comp, Expr_Node **node)
{
	switch ((*node)->kind)
	{
		case EXPR_IF:
			compiler_expr_if(comp, node);
			break;

		case EXPR_MATCH:
			compiler_expr_match(comp, node);
			break;

		case EXPR_LET:
			compiler_expr_let(comp, node);
			break;

		case EXPR_FUN:
			compiler_expr_fun(comp, node);
			break;

		case EXPR_LAMBDA:
			compiler_expr_lambda(comp, node);
			break;

		case EXPR_APP:
			compiler_expr_app(comp, node);
			break;

		case EXPR_IDENT:
			break;

		case EXPR_TUPLE:
			compiler_expr_tuple(comp, node);
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_HOT void
compiler_decl_let(Compiler *comp, Decl_Let *node)
{
	compiler_expr(comp, &node->value);

	Global glob = {
		.name = format_str(comp->arena, STR_STATIC("{S}@{u}"),
							node->name, compiler_id(comp)),
		.expr = node->value,
		.fun = false,
	};
	glob_buf_push(&comp->buf, glob);
}

static MAC_HOT void
compiler_decl_fun(Compiler *comp, Decl_Fun *node)
{
	compiler_expr(comp, &node->body);

	Global glob = {
		.name = format_str(comp->arena, STR_STATIC("{S}@{u}"),
							node->name, compiler_id(comp)),
		.expr = node->body,
		.fun = true,
	};
	glob_buf_push(&comp->buf, glob);
}

static MAC_HOT void
compiler_decl(Compiler *comp, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			compiler_decl_let(comp, (Decl_Let *)node);
			break;

		case DECL_FUN:
			compiler_decl_fun(comp, (Decl_Fun *)node);
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
compiler_init(Compiler *comp, Arena *arena, Hash_Default seed)
{
	comp->arena = arena;
	comp->err = NULL;
	comp->succ = true;

	comp->id = 0;
	glob_buf_init(&comp->buf);
}

void
compiler_free(Compiler *comp)
{
	glob_buf_free(&comp->buf);
}

MAC_HOT bool
compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err)
{
	comp->err = err;
	bool succ = true;

	for (size_t i = 0; i < ast->len; ++i)
	{
		compiler_decl(comp, ast->decls[i]);

		if (!comp->succ)
		{
			succ = false;
			comp->succ = true;
		}
	}

	return succ;
}
