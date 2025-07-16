#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nml_ast.h"
#include "nml_comp.h"
#include "nml_dbg.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_sym.h"
#include "nml_type.h"

static MAC_INLINE inline void
compiler_error(Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
}

static MAC_HOT inline void
compiler_if(Compiler *comp, Ast_If *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_match(Compiler *comp, Ast_Match *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_let(Compiler *comp, Ast_Let *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_fun(Compiler *comp, Ast_Fun *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_app(Compiler *comp, Ast_App *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_ident(Compiler *comp, Ast_Ident *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_unary(Compiler *comp, Ast_Unary *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_binary(Compiler *comp, Ast_Binary *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_tuple(Compiler *comp, Ast_Tuple *node, Ast_Tab *ctx)
{
}

static MAC_HOT inline void
compiler_const(Compiler *comp, Ast_Const *node, Ast_Tab *ctx)
{
}

static MAC_HOT void
compiler_node(Compiler *comp, Ast_Node *node, Ast_Tab *ctx)
{
	switch (node->kind)
	{
		case AST_IF:
			compiler_if(comp, (Ast_If *)node, ctx);
			break;

		case AST_MATCH:
			compiler_match(comp, (Ast_Match *)node, ctx);
			break;

		case AST_LET:
			compiler_let(comp, (Ast_Let *)node, ctx);
			break;

		case AST_FUN:
			compiler_fun(comp, (Ast_Fun *)node, ctx);
			break;

		case AST_APP:
			compiler_app(comp, (Ast_App *)node, ctx);
			break;

		case AST_IDENT:
			compiler_ident(comp, (Ast_Ident *)node, ctx);
			break;

		case AST_UNARY:
			compiler_unary(comp, (Ast_Unary *)node, ctx);
			break;

		case AST_BINARY:
			compiler_binary(comp, (Ast_Binary *)node, ctx);
			break;

		case AST_TUPLE:
			compiler_tuple(comp, (Ast_Tuple *)node, ctx);
			break;

		case AST_CONST:
			compiler_const(comp, (Ast_Const *)node, ctx);
			break;
	}
}

void
compiler_init(Compiler *comp, Arena *arena, Hash_Default seed)
{
	comp->arena = arena;
	comp->err = NULL;
	comp->succ = true;
	comp->seed = seed;
}

MAC_HOT bool
compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err)
{
	comp->err = err;
	bool succ = true;

	Ast_Tab ctx;
	ast_tab_init(&ctx, comp->seed);

	sym_tab_init(&comp->tab, comp->seed);
	for (size_t i = 0; i < ast->len; ++i)
	{
		compiler_node(comp, ast->items[i], &ctx);
		if (MAC_UNLIKELY(!comp->succ))
		{
			comp->succ = true;
			succ = false;
		}
	}

	return succ;
}
