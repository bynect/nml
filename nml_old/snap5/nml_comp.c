#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nml_ast.h"
#include "nml_comp.h"
#include "nml_mem.h"
#include "nml_type.h"

//static MAC_HOT void
//compiler_if(Compiler *comp, Expr_If *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_let(Compiler *comp, Expr_Let *node, bool top)
//{
//
//}
//
//static MAC_HOT void
//compiler_fun(Compiler *comp, Expr_Fun *node, bool top)
//{
//
//}
//
//static MAC_HOT void
//compiler_app(Compiler *comp, Expr_App *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_ident(Compiler *comp, Expr_Ident *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_unary(Compiler *comp, Expr_Unary *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_binary(Compiler *comp, Expr_Binary *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_tuple(Compiler *comp, Expr_Tuple *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_const(Compiler *comp, Expr_Const *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_node(Compiler *comp, Expr_Node *node)
//{
//	switch (node->kind)
//	{
//		case AST_IF:
//			compiler_if(comp, (Expr_If *)node);
//			break;
//
//		case AST_LET:
//			compiler_let(comp, (Expr_Let *)node, false);
//			break;
//
//		case AST_FUN:
//			compiler_fun(comp, (Expr_Fun *)node, false);
//			break;
//
//		case AST_APP:
//			compiler_app(comp, (Expr_App *)node);
//			break;
//
//		case AST_IDENT:
//			compiler_ident(comp, (Expr_Ident *)node);
//			break;
//
//		case AST_UNARY:
//			compiler_unary(comp, (Expr_Unary *)node);
//			break;
//
//		case AST_BINARY:
//			compiler_binary(comp, (Expr_Binary *)node);
//			break;
//
//		case AST_TUPLE:
//			compiler_tuple(comp, (Expr_Tuple *)node);
//			break;
//
//		case AST_CONST:
//			compiler_const(comp, (Expr_Const *)node);
//			break;
//	}
//}
//
//static MAC_INLINE inline void
//compiler_top(Compiler *comp, Expr_Node *node)
//{
//	if (node->kind == AST_LET)
//	{
//		compiler_let(comp, (Expr_Let *)node, true);
//	}
//	else if (node->kind == AST_FUN &&
//			((Expr_Fun *)node)->name.str[0] != '@')
//	{
//		compiler_fun(comp, (Expr_Fun *)node, true);
//	}
//	else
//	{
//		compiler_node(comp, node);
//	}
//}

void
compiler_init(Compiler *comp, Arena *arena)
{
	comp->arena = arena;
	comp->succ = false;
	comp->err = NULL;
}

MAC_HOT bool
compiler_compile(Compiler *comp, Ast_Module *ast, Error_List *err)
{
	comp->err = err;
	return false;
}
