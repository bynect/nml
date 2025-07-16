//#include "nml_inline.h"
//#include "nml_expr.h"
//#include "nml_hash.h"
//#include "nml_set.h"
//#include "nml_type.h"
//
//static MAC_HOT void inliner_expr(Inliner *inlin, Expr_Node *node);
//
//static MAC_HOT void
//inliner_if(Inliner *inlin, Expr_If *node)
//{
//	inliner_expr(inlin, node->cond);
//	inliner_expr(inlin, node->b_then);
//	inliner_expr(inlin, node->b_else);
//}
//
//static MAC_HOT void
//inliner_match(Inliner *inlin, Expr_Match *node)
//{
//	inliner_expr(inlin, node->value);
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		inliner_expr(inlin, node->arms[i].expr);
//	}
//}
//
//static MAC_HOT void
//inliner_let(Inliner *inlin, Expr_Let *node)
//{
//	inliner_expr(inlin, node->value);
//	inliner_expr(inlin, node->expr);
//}
//
//static MAC_HOT void
//inliner_fun(Inliner *inlin, Expr_Fun *node)
//{
//	inliner_expr(inlin, node->body);
//	inliner_expr(inlin, node->expr);
//}
//
//static MAC_HOT void
//inliner_lambda(Inliner *inlin, Expr_Lambda *node)
//{
//	inliner_expr(inlin, node->body);
//}
//
//static MAC_HOT void
//inliner_app(Inliner *inlin, Expr_App *node)
//{
//	inliner_expr(inlin, node->fun);
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		inliner_expr(inlin, node->args[i]);
//	}
//
//	if (node->fun->kind == EXPR_LAMBDA)
//	{
//		Expr_Lambda *fun = (Expr_Lambda *)node->fun;
//		size_t len = MAC_MIN(node->len, fun->len);
//
//		if (len != fun->len)
//		{
//			for (size_t i = len; i < fun->len; ++i)
//			{
//				str_set_insert(&inlin->set, fun->pars[i]);
//			}
//
//			for (size_t i = 0; i < len; ++i)
//			{
//				if (!str_set_member(&inlin->set, fun->pars[i]))
//				{
//					fun->body = expr_let_new(inlin->arena, fun->node.loc, TYPE_NONE,
//											fun->pars[i], node->args[i], fun->body);
//				}
//			}
//
//			fun->len -= len;
//			memmove(fun->pars, fun->pars + len, fun->len * sizeof(Sized_Str));
//
//			if (len != node->len)
//			{
//				node->len -= len;
//				memmove(node->args, node->args + len, node->len * sizeof(Expr_Node *));
//			}
//			else
//			{
//			}
//		}
//		else
//		{
//			for (size_t i = 0; i < len; ++i)
//			{
//				fun->body = expr_let_new(inlin->arena, fun->node.loc, TYPE_NONE,
//										fun->pars[i], node->args[i], fun->body);
//			}
//
//		}
//	}
//}
//
//static MAC_HOT void
//inliner_tuple(Inliner *inlin, Expr_Tuple *node)
//{
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		inliner_expr(inlin, node->items[i]);
//	}
//}
//
//static MAC_HOT void
//inliner_expr(Inliner *inlin, Expr_Node *node)
//{
//	switch (node->kind)
//	{
//		case EXPR_IF:
//			inliner_if(inlin, (Expr_If *)node);
//			break;
//
//		case EXPR_MATCH:
//			inliner_match(inlin, (Expr_Match *)node);
//			break;
//
//		case EXPR_LET:
//			inliner_let(inlin, (Expr_Let *)node);
//			break;
//
//		case EXPR_FUN:
//			inliner_fun(inlin, (Expr_Fun *)node);
//			break;
//
//		case EXPR_LAMBDA:
//			inliner_lambda(inlin, (Expr_Lambda *)node);
//			break;
//
//		case EXPR_APP:
//			inliner_app(inlin, (Expr_App *)node);
//			break;
//
//		case EXPR_IDENT:
//			break;
//
//		case EXPR_TUPLE:
//			inliner_tuple(inlin, (Expr_Tuple *)node);
//			break;
//
//		case EXPR_CONST:
//			break;
//	}
//}
//
//static MAC_HOT void
//inliner_decl(Inliner *inlin, Decl_Node *node)
//{
//	switch (node->kind)
//	{
//		case DECL_LET:
//			inliner_expr(inlin, MAC_CAST(Decl_Let *, node)->value);
//			break;
//
//		case DECL_FUN:
//			inliner_expr(inlin, MAC_CAST(Decl_Fun *, node)->body);
//			break;
//
//		case DECL_DATA:
//			break;
//
//		case DECL_TYPE:
//			break;
//	}
//}
//
//void
//inliner_init(Inliner *inlin, Arena *arena, Hash_Default seed)
//{
//	inlin->arena = arena;
//	str_set_init(&inlin->set, seed);
//}
//
//void
//inliner_free(Inliner *inlin)
//{
//	str_set_free(&inlin->set);
//}
//
//MAC_HOT void
//inliner_pass(Inliner *inlin, Ast_Top *ast)
//{
//	for (size_t i = 0; i < ast->len; ++i)
//	{
//		inliner_decl(inlin, ast->decls[i]);
//	}
//}
