#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nml_comp.h"
#include "nml_dbg.h"
#include "nml_decl.h"
#include "nml_expr.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"

static MAC_INLINE inline void
compiler_error(Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
}

static MAC_HOT void
compiler_expr_if(Compiler *comp, Expr_If *node)
{

}

static MAC_HOT void
compiler_expr_match(Compiler *comp, Expr_Match *node)
{
	compiler_error(comp, node->node.loc, STR_STATIC("Not implemented"));
}

static MAC_HOT void
compiler_expr_let(Compiler *comp, Expr_Let *node)
{
}

static MAC_HOT void
compiler_expr_fun(Compiler *comp, Expr_Fun *node)
{
}

static MAC_HOT void
compiler_expr_lambda(Compiler *comp, Expr_Lambda *node)
{
}

static MAC_HOT void
compiler_expr_app(Compiler *comp, Expr_App *node)
{
}

static MAC_HOT void
compiler_expr_ident(Compiler *comp, Expr_Ident *node)
{
}

static MAC_HOT void
compiler_expr_tuple(Compiler *comp, Expr_Tuple *node)
{
}

static MAC_HOT void
compiler_expr_const(Compiler *comp, Expr_Const *node)
{
}

static MAC_HOT void
compiler_expr(Compiler *comp, Expr_Node *node)
{
	switch (node->kind)
	{
		case EXPR_IF:
			compiler_expr_if(comp, (Expr_If *)node);
			break;

		case EXPR_MATCH:
			compiler_expr_match(comp, (Expr_Match *)node);
			break;

		case EXPR_LET:
			compiler_expr_let(comp, (Expr_Let *)node);
			break;

		case EXPR_FUN:
			compiler_expr_fun(comp, (Expr_Fun *)node);
			break;

		case EXPR_LAMBDA:
			compiler_expr_lambda(comp, (Expr_Lambda *)node);
			break;

		case EXPR_APP:
			compiler_expr_app(comp, (Expr_App *)node);
			break;

		case EXPR_IDENT:
			compiler_expr_ident(comp, (Expr_Ident *)node);
			break;

		case EXPR_TUPLE:
			compiler_expr_tuple(comp, (Expr_Tuple *)node);
			break;

		case EXPR_CONST:
			compiler_expr_const(comp, (Expr_Const *)node);
			break;
	}
}

static MAC_HOT void
compiler_decl_let(Compiler *comp, Decl_Let *node)
{
	Symbol sym = {
		.kind = SYM_GLOB,
		.expr = node->value,
	};

	if (sym_tab_unique(&comp->tab, node->name, sym) & TAB_FAIL)
	{
		compiler_error(comp, node->node.loc, STR_STATIC("Duplicate `let` declaration"));
	}
}

static MAC_HOT void
compiler_decl_fun(Compiler *comp, Decl_Fun *node)
{
	Symbol sym = {
		.kind = SYM_FUN,
		.expr = node->body,
	};

	if (sym_tab_unique(&comp->tab, node->name, sym) & TAB_FAIL)
	{
		compiler_error(comp, node->node.loc, STR_STATIC("Duplicate `fun` declaration"));
	}
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

	sym_tab_init(&comp->tab, seed);
}

void
compiler_free(Compiler *comp)
{
	sym_tab_free(&comp->tab);
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
