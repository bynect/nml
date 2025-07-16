#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nml_ast.h"
#include "nml_bc.h"
#include "nml_comp.h"
#include "nml_mem.h"
#include "nml_type.h"

//static MAC_HOT void
//compiler_if(Compiler *comp, Ast_If *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_let(Compiler *comp, Ast_Let *node, bool top)
//{
//
//}
//
//static MAC_HOT void
//compiler_fun(Compiler *comp, Ast_Fun *node, bool top)
//{
//
//}
//
//static MAC_HOT void
//compiler_app(Compiler *comp, Ast_App *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_ident(Compiler *comp, Ast_Ident *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_unary(Compiler *comp, Ast_Unary *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_binary(Compiler *comp, Ast_Binary *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_tuple(Compiler *comp, Ast_Tuple *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_const(Compiler *comp, Ast_Const *node)
//{
//
//}
//
//static MAC_HOT void
//compiler_node(Compiler *comp, Ast_Node *node)
//{
//	switch (node->kind)
//	{
//		case AST_IF:
//			compiler_if(comp, (Ast_If *)node);
//			break;
//
//		case AST_LET:
//			compiler_let(comp, (Ast_Let *)node, false);
//			break;
//
//		case AST_FUN:
//			compiler_fun(comp, (Ast_Fun *)node, false);
//			break;
//
//		case AST_APP:
//			compiler_app(comp, (Ast_App *)node);
//			break;
//
//		case AST_IDENT:
//			compiler_ident(comp, (Ast_Ident *)node);
//			break;
//
//		case AST_UNARY:
//			compiler_unary(comp, (Ast_Unary *)node);
//			break;
//
//		case AST_BINARY:
//			compiler_binary(comp, (Ast_Binary *)node);
//			break;
//
//		case AST_TUPLE:
//			compiler_tuple(comp, (Ast_Tuple *)node);
//			break;
//
//		case AST_CONST:
//			compiler_const(comp, (Ast_Const *)node);
//			break;
//	}
//}
//
//static MAC_INLINE inline void
//compiler_top(Compiler *comp, Ast_Node *node)
//{
//	if (node->kind == AST_LET)
//	{
//		compiler_let(comp, (Ast_Let *)node, true);
//	}
//	else if (node->kind == AST_FUN &&
//			((Ast_Fun *)node)->name.str[0] != '@')
//	{
//		compiler_fun(comp, (Ast_Fun *)node, true);
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
compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err)
{
	comp->err = err;
	return false;
}
