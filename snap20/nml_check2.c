//#include <stdbool.h>
//#include <stddef.h>
//
//#include "nml_err.h"
//#include "nml_expr.h"
//#include "nml_patn.h"
//#include "nml_type.h"
//#include "nml_check.h"
//#include "nml_ast.h"
//#include "nml_mac.h"
//
//static MAC_INLINE inline Type
//checker_meta_new(Type_Checker *check)
//{
//
//}
//
//static MAC_INLINE inline Type
//checker_skolem_new(Type_Checker *check)
//{
//}
//
//static MAC_INLINE inline Type
//checker_var_new(Type_Checker *check)
//{
//}
//
//MAC_HOT void
//checker_infer_patn(Type_Checker *check, Patn_Node *node)
//{
//	Type type = checker_infer_sigma(check, node);
//	node->type = checker_zonk(type);
//
//	if (node->kind == PATN_TUPLE)
//	{
//		Patn_Tuple *tuple = (Patn_Tuple *)node;
//		for (size_t i = 0; i < tuple->len; ++i)
//		{
//			checker_infer_patn(check, tuple->items[i]);
//		}
//	}
//}
//
//MAC_HOT void
//checker_infer_expr(Type_Checker *check, Expr_Node *node)
//{
//	Type type = checker_infer_sigma(check, node);
//	node->type = checker_zonk(type);
//
//	switch (node->kind)
//	{
//		case EXPR_IF:
//			checker_infer_expr(check, MAC_CAST(Expr_If *, node)->cond);
//			checker_infer_expr(check, MAC_CAST(Expr_If *, node)->b_then);
//			checker_infer_expr(check, MAC_CAST(Expr_If *, node)->b_else);
//			break;
//
//		case EXPR_MATCH:
//			checker_infer_expr(check, MAC_CAST(Expr_Match *, node)->value);
//
//			for (size_t i = 0; i < MAC_CAST(Expr_Match *, node)->len; ++i)
//			{
//				checker_infer_patn(check, MAC_CAST(Expr_Match *, node)->arms[i].patn);
//				checker_infer_expr(check, MAC_CAST(Expr_Match *, node)->arms[i].expr);
//			}
//			break;
//
//		case EXPR_LET:
//			checker_infer_expr(check, MAC_CAST(Expr_Let *, node)->value);
//			if (MAC_CAST(Expr_Let *, node)->expr != NULL)
//			{
//				checker_infer_expr(check, MAC_CAST(Expr_Let *, node)->expr);
//			}
//			break;
//
//		case EXPR_FUN:
//			checker_infer_expr(check, MAC_CAST(Expr_Fun *, node)->body);
//			if (MAC_CAST(Expr_Fun *, node)->expr != NULL)
//			{
//				checker_infer_expr(check, MAC_CAST(Expr_Fun *, node)->expr);
//			}
//			break;
//
//		case EXPR_APP:
//			checker_infer_expr(check, MAC_CAST(Expr_App *, node)->fun);
//			for (size_t i = 0; i < MAC_CAST(Expr_App *, node)->len; ++i)
//			{
//				checker_infer_expr(check, MAC_CAST(Expr_App *, node)->args[i]);
//			}
//			break;
//
//		case EXPR_IDENT:
//			break;
//
//		case EXPR_BINARY:
//			checker_infer_expr(check, MAC_CAST(Expr_Binary *, node)->rhs);
//			checker_infer_expr(check, MAC_CAST(Expr_Binary *, node)->lhs);
//			break;
//
//		case EXPR_TUPLE:
//			for (size_t i = 0; i < MAC_CAST(Expr_Tuple *, node)->len; ++i)
//			{
//				checker_infer_expr(check, MAC_CAST(Expr_Tuple *, node)->items[i]);
//			}
//			break;
//
//		case EXPR_CONST:
//			break;
//	}
//}
//
//MAC_HOT bool
//checker_check(Type_Checker *check, Ast_Top *ast, Error_List *err)
//{
//	bool succ = true;
//
//	for (size_t i = 0; i < ast->len; ++i)
//	{
//		if (ast->items[i].kind == AST_EXPR)
//		{
//			checker_infer_expr(check, ast->items[i].node);
//		}
//
//		if (!check->succ)
//		{
//			check->succ = true;
//			succ = false;
//		}
//	}
//
//	return succ;
//}
