//#include <stdbool.h>
//#include <stddef.h>
//#include <stdio.h>
//
//#include "nml_ast.h"
//#include "nml_dbg.h"
//#include "nml_mac.h"
//#include "nml_mem.h"
//#include "nml_str.h"
//#include "nml_type.h"
//#include "nml_buf.h"
//
//static Type
//checker_apply_subst(Type_Tab *subst, Type type)
//{
//	const Type_Tag tag = TYPE_UNTAG(type);
//	if (tag == TAG_VAR)
//	{
//		Type var;
//		if (type_tab_get(subst, ((Type_Var *)TYPE_PTR(type))->str, &var))
//		{
//			return var;
//		}
//	}
//	else if (tag == TAG_FUN)
//	{
//		Type_Fun *fun = TYPE_PTR(type);
//		for (size_t i = 0; i < fun->len; ++i)
//		{
//			fun->pars[i] = checker_apply_subst(subst, fun->pars[i]);
//		}
//
//		fun->ret = checker_apply_subst(subst, fun->ret);
//	}
//	else if (tag == TAG_TUPLE)
//	{
//		Type_Tuple *tuple = TYPE_PTR(type);
//		for (size_t i = 0; i < tuple->len; ++i)
//		{
//			tuple->items[i]	= checker_apply_subst(subst, tuple->items[i]);
//		}
//	}
//
//	return type;
//}
//
//static void
//checker_apply_scheme(Type_Tab *subst, Type_Scheme *scheme)
//{
//	Sized_Str *vars = scheme->vars;
//	for (size_t i = scheme->len; i >  0; --i)
//	{
//		type_tab_del(subst, vars[i - 1]);
//	}
//
//	scheme->type = checker_apply_subst(subst, scheme->type);
//}
//
//static MAC_INLINE inline Type
//checker_newvar(Type_Checker *check)
//{
//	Type_Var *var = arena_alloc(check->arena, sizeof(Type_Var));
////	var->str = str_from_int_prefix(check->arena, check->tid++, 10, STR_STATIC("T"));
//	return TYPE_TAG(TAG_VAR, var);
//}
//
//static void
//checker_freevar(Str_Buf *vars, Type type)
//{
//	const Type_Tag tag = TYPE_UNTAG(type);
//	if (tag == TAG_VAR)
//	{
//		Type_Var *var = TYPE_PTR(type);
//		str_buf_unique(vars, var->str);
//	}
//	else if (tag == TAG_FUN)
//	{
//		Type_Fun *fun = TYPE_PTR(type);
//		for (size_t i = 0; i < fun->len; ++i)
//		{
//			checker_freevar(vars, fun->pars[i]);
//		}
//
//		checker_freevar(vars, fun->ret);
//	}
//	else if (tag == TAG_TUPLE)
//	{
//		Type_Tuple *tuple = TYPE_PTR(type);
//		for (size_t i = 0; i < tuple->len; ++i)
//		{
//			checker_freevar(vars, tuple->items[i]);
//		}
//	}
//}
//
//static void
//checker_schemevar(Type_Checker *check, Type_Scheme *scheme, Str_Buf *res)
//{
//	Str_Buf vars;
//	str_buf_init(&vars, check->arena);
//	checker_freevar(&vars, scheme->type);
//
//	for (size_t i = 0; i < vars.len; ++i)
//	{
//		size_t eq = 0;
//		for (size_t j = 0; j < scheme->len; ++j)
//		{
//			if (STR_EQUAL(vars.buf[i], scheme->vars[j]))
//			{
//				++eq;
//			}
//		}
//
//		if (eq != scheme->len)
//		{
//			str_buf_push(res, vars.buf[i]);
//		}
//	}
//
//	str_buf_free(&vars);
//}
//
//static bool
//checker_bindvar(Type_Checker *check, Sized_Str var, Type type, Type_Tab *subst)
//{
//	if (TYPE_UNTAG(type) == TAG_VAR &&
//		STR_EQUAL(((Type_Var *)TYPE_PTR(type))->str, var))
//	{
//		return true;
//	}
//
//	Str_Buf vars;
//	str_buf_init(&vars, check->arena);
//	checker_freevar(&vars, type);
//
//	if (str_buf_contain(&vars, var))
//	{
//		printf("Occurs check failed\n");
//		str_buf_free(&vars);
//		return false;
//	}
//
//	type_tab_set(subst, check->arena, var, type);
//	str_buf_free(&vars);
//	return true;
//}
//
//static bool
//checker_unify(Type_Checker *check, Type type1, Type type2, Type_Tab *subst)
//{
//	const Type_Tag tag1 = TYPE_UNTAG(type1);
//	const Type_Tag tag2 = TYPE_UNTAG(type2);
//
//	switch (tag1)
//	{
//		case TAG_FUN:
//			if (tag2 == TAG_FUN)
//			{
//				Type_Fun *fun1 = TYPE_PTR(type1);
//				Type_Fun *fun2 = TYPE_PTR(type2);
//
//				if (fun1->len != fun2->len)
//				{
//					break;
//				}
//
//				for (size_t i = 0; fun1->len; ++i)
//				{
//					if (!checker_unify(check, fun1->pars[i], fun2->pars[i], subst))
//					{
//						break;
//					}
//				}
//
//				Type ret1 = checker_apply_subst(subst, fun1->ret);
//				Type ret2 = checker_apply_subst(subst, fun2->ret);
//
//				if (!checker_unify(check, ret1, ret2, subst))
//				{
//					return false;
//				}
//
//				return true;
//			}
//			break;
//
//		case TAG_TUPLE:
//			if (tag2 == TAG_TUPLE)
//			{
//				Type_Tuple *tuple1 = TYPE_PTR(type1);
//				Type_Tuple *tuple2 = TYPE_PTR(type2);
//
//				if (tuple1->len != tuple2->len)
//				{
//					break;
//				}
//
//				for (size_t i = 0; tuple1->len; ++i)
//				{
//					if (!checker_unify(check, tuple1->items[i], tuple2->items[i], subst))
//					{
//						break;
//					}
//				}
//				return true;
//			}
//			break;
//
//		case TAG_VAR:
//			return checker_bindvar(check, ((Type_Var *)TYPE_PTR(type1))->str, type2, subst);
//
//		default:
//			if (tag2 == TAG_VAR)
//			{
//				return checker_bindvar(check, ((Type_Var *)TYPE_PTR(type2))->str, type1, subst);
//			}
//			else if (tag1 == tag2)
//			{
//				return true;
//			}
//			break;
//	}
//
//	printf("Types cannot be unified\n");
//	dbg_dump_type(type1);
//	dbg_dump_type(type2);
//	return false;
//}
//
//static void
//checker_apply_iter(Scheme_Tab_Buck *buck, void *ctx)
//{
//	if (buck == NULL)
//	{
//		return;
//	}
//
//	checker_apply_scheme(ctx, &buck->v);
//}
//
//static void
//checker_apply_ctx(Type_Tab *subst, Scheme_Tab *ctx)
//{
//	scheme_tab_iter(ctx, checker_apply_iter, subst);
//}
//
//static Type
//checker_instantiate(Type_Checker *check, Type_Scheme *scheme)
//{
//	Type_Tab subst;
//	type_tab_init(&subst, 0);
//
//	for (size_t i = 0; i < scheme->len; ++i)
//	{
//		Type type = checker_newvar(check);
//		type_tab_set(&subst, check->arena, scheme->vars[i], type);
//	}
//
//	return checker_apply_subst(&subst, scheme->type);
//}
//
//static Type checker_infer(Type_Checker *check, const Ast_Node *node, Scheme_Tab *ctx, Type_Tab *subst);
//
//static Type
//checker_infer_if(Type_Checker *check, const Ast_If *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	Type cond = checker_infer(check, node->cond, ctx, subst);
//	Type then = checker_infer(check, node->then, ctx, subst);
//	Type m_else = node->m_else == NULL ?
//				TYPE_UNIT : checker_infer(check, node->m_else, ctx, subst);
//
//	if (!checker_unify(check, cond, TYPE_BOOL, subst) ||
//		!checker_unify(check, then, m_else, subst))
//	{
//		return TYPE_NONE;
//	}
//	return checker_apply_subst(subst, then);
//}
//
//static Type
//checker_infer_let(Type_Checker *check, const Ast_Let *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	Type value = checker_infer(check, node->value, ctx, subst);
//	Type_Scheme scheme = {
//		.vars = NULL,
//		.len = 0,
//		.type = checker_apply_subst(subst, value),
//	};
//
//	if (node->expr != NULL)
//	{
//		Scheme_Tab tmp;
//		scheme_tab_init(&tmp, 0);
//		scheme_tab_merge(ctx, &tmp, check->arena);
//
//		scheme_tab_set(&tmp, check->arena, node->name, scheme);
//		checker_apply_ctx(subst, &tmp);
//		return checker_infer(check, node->expr, &tmp, subst);
//	}
//	return value;
//}
//
//static Type
//checker_infer_fun(Type_Checker *check, const Ast_Fun *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	Scheme_Tab tmp;
//	scheme_tab_init(&tmp, 0);
//	scheme_tab_merge(ctx, &tmp, check->arena);
//
//	Type *pars = arena_alloc(check->arena, node->pars_len * sizeof(Type));
//	for (size_t i = 0; i < node->pars_len; ++i)
//	{
//		pars[i] = checker_newvar(check);
//		Type_Scheme scheme = {
//			.vars = NULL,
//			.len = 0,
//			.type = pars[i],
//		};
//		scheme_tab_set(&tmp, check->arena, node->pars[i], scheme);
//	}
//
//	Type body = checker_infer(check, node->body, &tmp, subst);
//	for (size_t i = 0; i < node->pars_len; ++i)
//	{
//		pars[i] = checker_apply_subst(subst, pars[i]);
//	}
//
//	Type_Fun *fun_ptr = arena_alloc(check->arena, sizeof(Type_Fun));
//	fun_ptr->pars = pars;
//	fun_ptr->len = node->pars_len;
//	fun_ptr->ret = body;
//
//	if (node->expr != NULL)
//	{
//		checker_infer_type(check, node->expr);
//	}
//	return TYPE_TAG(TAG_FUN, fun_ptr);
//}
//
//static Type
//checker_infer_app(Type_Checker *check, const Ast_App *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	Type ret = checker_newvar(check);
//	Type fun = checker_infer(check, node->fun, ctx, subst);
//	checker_apply_ctx(subst, ctx);
//
//	Type_Fun fun_app = {
//		.pars = arena_alloc(check->arena, node->len * sizeof(Type)),
//		.len = node->len,
//		.ret = ret,
//	};
//
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		fun_app.pars[i] = checker_infer(check, node->args[i], ctx, subst);
//	}
//
//	fun = checker_apply_subst(subst, fun);
//	Type fun_type = TYPE_TAG(TAG_FUN, &fun_app);
//
//	if (!checker_unify(check, fun, fun_type, subst))
//	{
//		return TYPE_NONE;
//	}
//	return checker_apply_subst(subst, ret);
//}
//
//static Type
//checker_infer_ident(Type_Checker *check, Ast_Ident *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	Type_Scheme scheme;
//	if (!scheme_tab_get(ctx, node->name, &scheme))
//	{
//		printf("Unbound identifier ");
//		dbg_dump_sized_str(node->name);
//		return TYPE_NONE;
//	}
//	return checker_instantiate(check, &scheme);
//}
//
//static Type
//checker_infer_unary(Type_Checker *check, Ast_Unary *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	const Type types[] = {
//		[TOK_MINUS] = TYPE_INT,
//		[TOK_BANG] = TYPE_BOOL,
//	};
//
//	Type lhs = checker_infer(check, node->lhs, ctx, subst);
//	if (!checker_unify(check, lhs, types[node->op], subst))
//	{
//		return TYPE_NONE;
//	}
//	return checker_apply_subst(subst, lhs);
//}
//
//static Type
//checker_infer_binary(Type_Checker *check, Ast_Binary *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	const Sized_Str toks[] = {
//#define TOK(type, str, _, kind) [TOK_ ## type] = STR_STATIC(str),
//#define OPER
//#include "nml_tok.h"
//#undef OPER
//#undef TOK
//	};
//
//	const Ast_Ident fun = {
//		.node = {
//			.kind = AST_IDENT,
//			.type = TYPE_NONE,
//		},
//		.name = toks[node->op],
//	};
//
//	const Ast_App app = {
//		.node = {
//			.kind = AST_APP,
//			.type = TYPE_NONE,
//		},
//		.fun = (Ast_Node *)&fun,
//		.args = (Ast_Node *[]) { node->rhs, node->lhs },
//		.len = 2,
//	};
//
//	Type type = checker_infer_app(check, &app, ctx, subst);
//	return type;
//}
//
//static Type
//checker_infer_tuple(Type_Checker *check, Ast_Tuple *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	Type_Tuple *tuple = arena_alloc(check->arena, sizeof(Type_Tuple));
//	tuple->items = arena_alloc(check->arena, node->len * sizeof(Type));
//	tuple->len = node->len;
//
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		tuple->items[i] = checker_infer(check, node->values[i], ctx, subst);
//	}
//	return TYPE_TAG(TAG_TUPLE, tuple);
//}
//
//static Type
//checker_infer_const(const Ast_Const *node)
//{
//	const Type consts[] = {
//		[CONST_UNIT] = TYPE_UNIT,
//		[CONST_INT] = TYPE_INT,
//		[CONST_FLOAT] = TYPE_FLOAT,
//		[CONST_STR] = TYPE_STR,
//		[CONST_CHAR] = TYPE_CHAR,
//		[CONST_BOOL] = TYPE_BOOL,
//	};
//	return consts[node->kind];
//}
//
//static MAC_HOT Type
//checker_infer(Type_Checker *check, const Ast_Node *node, Scheme_Tab *ctx, Type_Tab *subst)
//{
//	switch (node->kind)
//	{
//		case AST_IF:
//			return checker_infer_if(check, (Ast_If *)node, ctx, subst);
//
//		case AST_LET:
//			return checker_infer_let(check, (Ast_Let *)node, ctx, subst);
//
//		case AST_FUN:
//			return checker_infer_fun(check, (Ast_Fun *)node, ctx, subst);
//
//		case AST_APP:
//			return checker_infer_app(check, (Ast_App *)node, ctx, subst);
//
//		case AST_IDENT:
//			return checker_infer_ident(check, (Ast_Ident *)node, ctx, subst);
//
//		case AST_UNARY:
//			return checker_infer_unary(check, ((Ast_Unary *)node), ctx, subst);
//
//		case AST_BINARY:
//			return checker_infer_binary(check, ((Ast_Binary *)node), ctx, subst);
//
//		case AST_TUPLE:
//			return checker_infer_tuple(check, ((Ast_Tuple *)node), ctx, subst);
//
//		case AST_CONST:
//			return checker_infer_const((Ast_Const *)node);
//	}
//	return TYPE_NONE;
//}
//
//void
//checker_init(Type_Checker *check, Arena *arena, Scheme_Tab *ctx)
//{
//	check->tid = 0;
//	check->arena = arena;
//
//	type_tab_init(&check->subst, 0);
//	if (MAC_UNLIKELY(ctx == NULL))
//	{
//		ctx = arena_alloc(arena, sizeof(Scheme_Tab));
//		scheme_tab_init(ctx, 0);
//	}
//	check->ctx = ctx;
//}
//
//MAC_HOT Type
//checker_infer_type(Type_Checker *check, const Ast_Node *node)
//{
//	Type_Tab subst;
//	type_tab_init(&subst, 0);
//	Type type = checker_infer(check, node, check->ctx, &subst);
//	return checker_apply_subst(&subst, type);
//}
