//#include <stdbool.h>
//#include <stddef.h>
//#include <stdint.h>
//#include <stdio.h>
//#include <stdlib.h>
//
//#include "nml_buf.h"
//#include "nml_err.h"
//#include "nml_type.h"
//#include "nml_mac.h"
//#include "nml_str.h"
//#include "nml_mem.h"
//#include "nml_ast.h"
//
//static Type checker_infer(Type_Checker *check, Ast_Node *node, Type_Tab *subst, Scheme_Tab *ctx);
//
//static MAC_INLINE inline Type
//checker_fun_new(Type_Checker *check, Type *pars, size_t len, Type ret)
//{
//	Type_Fun *fun = arena_alloc(check->arena, sizeof(Type_Fun));
//	fun->pars = pars;
//	fun->len = len;
//	fun->ret = ret;
//	return TYPE_TAG(TAG_FUN, fun);
//}
//
//
//static MAC_INLINE inline Type
//checker_tuple_new(Type_Checker *check, Type *items, size_t len)
//{
//	Type_Tuple *tuple = arena_alloc(check->arena, sizeof(Type_Tuple));
//	tuple->items = items;
//	tuple->len = len;
//	return TYPE_TAG(TAG_TUPLE, tuple);
//}
//
//static MAC_INLINE inline Type
//checker_var_new(Type_Checker *check)
//{
//	Type_Var *var = arena_alloc(check->arena, sizeof(Type_Var));
//	var->str = STR_STATIC("T");
//	str_append_int(&var->str, check->arena, check->tid++, 10);
//	return TYPE_TAG(TAG_VAR, var);
//}
//
//static MAC_HOT void
//checker_var_free(Str_Buf *vars, Type type)
//{
//	const Type_Tag tag = TYPE_UNTAG(type);
//
//	if (tag == TAG_FUN)
//	{
//		Type_Fun *fun = TYPE_PTR(type);
//		for (size_t i = 0; i < fun->len; ++i)
//		{
//			checker_var_free(vars, fun->pars[i]);
//		}
//
//		checker_var_free(vars, fun->ret);
//	}
//	else if (tag == TAG_TUPLE)
//	{
//		Type_Tuple *tuple = TYPE_PTR(type);
//		for (size_t i = 0; i < tuple->len; ++i)
//		{
//			checker_var_free(vars, tuple->items[i]);
//		}
//	}
//	else if (tag == TAG_VAR)
//	{
//		Type_Var *var = TYPE_PTR(type);
//		str_buf_unique(vars, var->str);
//	}
//}
//
//static MAC_HOT bool
//checker_var_bind(Type_Checker *check, Sized_Str var, Type type, Type_Tab *subst)
//{
//	if (TYPE_UNTAG(type) == TAG_VAR &&
//		STR_EQUAL(((Type_Var *)TYPE_PTR(type))->str, var))
//	{
//		return true;
//	}
//
//	Str_Buf vars;
//	str_buf_init(&vars, check->arena);
//	checker_var_free(&vars, type);
//
//	if (str_buf_contain(&vars, var))
//	{
//		error_append(check->err, check->arena,
//					STR_STATIC("Occurs check failed"), 0, ERR_ERROR);
//		str_buf_free(&vars);
//		return false;
//	}
//
//	type_tab_set(subst, check->arena, var, type);
//	str_buf_free(&vars);
//	return true;
//}
//
//static MAC_HOT Type
//checker_subst(Type_Checker *check, Type type, const Type_Tab *subst)
//{
//	const Type_Tag tag = TYPE_UNTAG(type);
//
//	if (tag == TAG_FUN)
//	{
//		Type_Fun *fun = TYPE_PTR(type);
//		Type *pars = arena_alloc(check->arena, fun->len * sizeof(Type));
//
//		for (size_t i = 0; i < fun->len; ++i)
//		{
//			pars[i] = checker_subst(check, fun->pars[i], subst);
//		}
//
//		Type ret = checker_subst(check, fun->ret, subst);
//		return checker_fun_new(check, pars, fun->len, ret);
//	}
//	else if (tag == TAG_TUPLE)
//	{
//		Type_Tuple *tuple = TYPE_PTR(type);
//		Type *items = arena_alloc(check->arena, tuple->len * sizeof(Type));
//
//		for (size_t i = 0; i < tuple->len; ++i)
//		{
//			items[i] = checker_subst(check, tuple->items[i], subst);
//		}
//
//		return checker_tuple_new(check, items, tuple->len);
//	}
//	else if (MAC_LIKELY(tag == TAG_VAR))
//	{
//		Type var;
//		if (type_tab_get(subst, ((Type_Var *)TYPE_PTR(type))->str, &var))
//		{
//			return var;
//		}
//	}
//
//	return type;
//}
//
//
//static void
//checker_subst_scheme(Type_Checker *check, Type_Tab *subst, Type_Scheme *scheme)
//{
//	Sized_Str *vars = scheme->vars;
//	for (size_t i = scheme->len; i >  0; --i)
//	{
//		type_tab_del(subst, vars[i - 1]);
//	}
//
//	scheme->type = checker_subst(check, scheme->type, subst);
//}
//
//static void
//checker_subst_ctx(Type_Checker *check, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	Type_Tab s1;
//	type_tab_init(&s1, 0);
//	type_tab_merge(subst, &s1, check->arena);
//
//	for (size_t i = 0; i < ctx->size; ++i)
//	{
//		if (ctx->bucks[i].w_key)
//		{
//			checker_subst_scheme(check, &s1, &ctx->bucks[i].v);
//		}
//	}
//}
//
//static void
//checker_subst_merge(Type_Checker *check, Type_Tab *s1, const Type_Tab *s2)
//{
//	for (size_t i = 0; i < s2->size; ++i)
//	{
//		if (s2->bucks[i].w_key)
//		{
//			Type type = checker_subst(check, s2->bucks[i].v, s1);
//			type_tab_set(s1, check->arena, s2->bucks[i].hk.k, type);
//		}
//	}
//}
//
//static MAC_HOT bool
//checker_unify(Type_Checker *check, Type t1, Type t2, Type_Tab *subst)
//{
//	const Type_Tag tag1 = TYPE_UNTAG(t1);
//	const Type_Tag tag2 = TYPE_UNTAG(t2);
//
//	printf("UNIF TYPE 1: ");
//	dbg_dump_type(t1);
//	printf("UNIF TYPE 2: ");
//	dbg_dump_type(t2);
//
//	if (tag1 == TAG_NONE || tag2 == TAG_NONE);// abort();
//
//	if (tag1 == TAG_FUN && tag2 == TAG_FUN)
//	{
//		Type_Fun *fun1 = TYPE_PTR(t1);
//		Type_Fun *fun2 = TYPE_PTR(t2);
//
//		if (fun1->len != fun2->len)
//		{
//			goto err;
//		}
//
//		for (size_t i = 0; fun1->len; ++i)
//		{
//			if (!checker_unify(check, fun1->pars[i], fun2->pars[i], subst))
//			{
//				goto err;
//			}
//		}
//
//		Type ret1 = checker_subst(check, fun1->ret, subst);
//		Type ret2 = checker_subst(check, fun2->ret, subst);
//		return checker_unify(check, ret1, ret2, subst);
//	}
//	else if (tag1 == TAG_TUPLE && tag2 == TAG_TUPLE)
//	{
//		Type_Tuple *tuple1 = TYPE_PTR(t1);
//		Type_Tuple *tuple2 = TYPE_PTR(t2);
//
//		if (tuple1->len != tuple2->len)
//		{
//			goto err;
//		}
//
//		for (size_t i = 0; tuple1->len; ++i)
//		{
//			if (!checker_unify(check, tuple1->items[i], tuple2->items[i], subst))
//			{
//				goto err;
//			}
//		}
//		return true;
//	}
//	else if (tag1 == TAG_VAR)
//	{
//		return checker_var_bind(check, ((Type_Var *)TYPE_PTR(t1))->str, t2, subst);
//	}
//	else if (tag2 == TAG_VAR)
//	{
//		return checker_var_bind(check, ((Type_Var *)TYPE_PTR(t2))->str, t1, subst);
//	}
//	else if (tag1 == tag2)
//	{
//		return true;
//	}
//
//err:
//	error_append(check->err, check->arena, STR_STATIC("Unification failed"), 0, ERR_ERROR);
//	return false;
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
//		Type type = checker_var_new(check);
//		type_tab_set(&subst, check->arena, scheme->vars[i], type);
//	}
//
//	return checker_subst(check, scheme->type, &subst);
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_if(Type_Checker *check, Ast_If *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	Type cond = checker_infer(check, node->cond, subst, ctx);
//	Type then = checker_infer(check, node->then, subst, ctx);
//	Type m_else = node->m_else == NULL ?
//				TYPE_UNIT : checker_infer(check, node->m_else, subst, ctx);
//
//	if (!checker_unify(check, cond, TYPE_BOOL, subst) ||
//		!checker_unify(check, then, m_else, subst))
//	{
//		return TYPE_NONE;
//	}
//	return checker_subst(check, then, subst);
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_let(Type_Checker *check, Ast_Let *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	Type value = checker_infer(check, node->value, subst, ctx);
//	value = checker_subst(check, value, subst);
//
//	if (node->expr != NULL)
//	{
//		Scheme_Tab scope;
//		scheme_tab_init(&scope, 0);
//		scheme_tab_merge(ctx, &scope, check->arena);
//
//		Type_Scheme scheme = SCHEME_EMPTY(value);
//		scheme_tab_set(&scope, check->arena, node->name, scheme);
//
//		checker_subst_ctx(check, subst, &scope);
//		return checker_infer(check, node->expr, subst, &scope);
//	}
//	return value;
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_fun(Type_Checker *check, Ast_Fun *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	Scheme_Tab scope;
//	scheme_tab_init(&scope, 0);
//	scheme_tab_merge(ctx, &scope, check->arena);
//
//	Type *pars = arena_alloc(check->arena, node->pars_len * sizeof(Type));
//	for (size_t i = 0; i < node->pars_len; ++i)
//	{
//		pars[i] = checker_var_new(check);
//		Type_Scheme scheme = SCHEME_EMPTY(pars[i]);
//		scheme_tab_set(&scope, check->arena, node->pars[i], scheme);
//	}
//
//	Type body = checker_infer(check, node->body, subst, &scope);
//	for (size_t i = 0; i < node->pars_len; ++i)
//	{
//		pars[i] = checker_subst(check, pars[i], subst);
//	}
//
//	Type fun = checker_fun_new(check, pars, node->pars_len, body);
//	if (node->expr != NULL)
//	{
//		for (size_t i = 0; i < node->pars_len; ++i)
//		{
//			scheme_tab_del(&scope, node->pars[i]);
//		}
//
//		Type_Scheme scheme = SCHEME_EMPTY(fun);
//		scheme_tab_set(&scope, check->arena, node->name, scheme);
//
//		checker_subst_ctx(check, subst, &scope);
//		return checker_infer(check, node->expr, subst, &scope);
//	}
//	return fun;
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_app(Type_Checker *check, Ast_App *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	Type ret = checker_var_new(check);
//
//	Type fun = checker_infer(check, node->fun, subst, ctx);
//	checker_subst_ctx(check, subst, ctx);
//
//	Type *args = arena_alloc(check->arena, node->len * sizeof(Type));
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		args[i] = checker_infer(check, node->args[i], subst, ctx);
//	}
//
//	Type app = checker_fun_new(check, args, node->len, ret);
//	fun = checker_subst(check, fun, subst);
//
//	if (!checker_unify(check, app, fun, subst))
//	{
//		return TYPE_NONE;
//	}
//
//	return checker_subst(check, ret, subst);
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_ident(Type_Checker *check, Ast_Ident *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	MAC_UNUSED(subst);
//	Type_Scheme scheme;
//
//	if (!scheme_tab_get(ctx, node->name, &scheme))
//	{
//		Sized_Str msg = str_format(check->arena, "Unbound identifier `%.*s`",
//									(uint32_t)node->name.len, node->name.str);
//		error_append(check->err, check->arena, msg, 0, ERR_ERROR);
//		return TYPE_NONE;
//	}
//	return checker_instantiate(check, &scheme);
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_unary(Type_Checker *check, Ast_Unary *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	const Sized_Str toks[] = {
//		[TOK_MINUS] = STR_STATIC("~-"),
//		[TOK_BANG] = STR_STATIC("~!"),
//	};
//
//	Ast_Ident fun = {
//		.node = {
//			.kind = AST_IDENT,
//			.type = TYPE_NONE,
//		},
//		.name = toks[node->op],
//	};
//
//	Ast_Node *args[] = {node->lhs};
//	Ast_App app = {
//		.node = {
//			.kind = AST_APP,
//			.type = TYPE_NONE,
//		},
//		.fun = (Ast_Node *)&fun,
//		.args = args,
//		.len = 1,
//	};
//
//	return checker_infer_app(check, &app, subst, ctx);
//}
//
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_binary(Type_Checker *check, Ast_Binary *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	const Sized_Str toks[] = {
//#define TOK(type, str, _, kind) [TOK_ ## type] = STR_STATIC(str),
//#define OPER
//#include "nml_tok.h"
//#undef OPER
//#undef TOK
//	};
//
//	Ast_Ident fun = {
//		.node = {
//			.kind = AST_IDENT,
//			.type = TYPE_NONE,
//		},
//		.name = toks[node->op],
//	};
//
//	Ast_Node *args[] = {node->rhs, node->lhs};
//	Ast_App app = {
//		.node = {
//			.kind = AST_APP,
//			.type = TYPE_NONE,
//		},
//		.fun = (Ast_Node *)&fun,
//		.args = args,
//		.len = 2,
//	};
//
//	return checker_infer_app(check, &app, subst, ctx);
//}
//
//static MAC_HOT MAC_INLINE inline Type
//checker_infer_tuple(Type_Checker *check, Ast_Tuple *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	Type *items = arena_alloc(check->arena, node->len * sizeof(Type));
//
//	for (size_t i = 0; i < node->len; ++i)
//	{
//		items[i] = checker_infer(check, node->values[i], subst, ctx);
//	}
//	return checker_tuple_new(check, items, node->len);
//}
//
//static MAC_HOT Type
//checker_infer(Type_Checker *check, Ast_Node *node, Type_Tab *subst, Scheme_Tab *ctx)
//{
//	const Type consts[] = {
//		[CONST_UNIT] = TYPE_UNIT,
//		[CONST_INT] = TYPE_INT,
//		[CONST_FLOAT] = TYPE_FLOAT,
//		[CONST_STR] = TYPE_STR,
//		[CONST_CHAR] = TYPE_CHAR,
//		[CONST_BOOL] = TYPE_BOOL,
//	};
//	Type type = TYPE_NONE;
//
//	switch (node->kind)
//	{
//		case AST_IF:
//			type = checker_infer_if(check, (Ast_If *)node, subst, ctx);
//			break;
//
//		case AST_LET:
//			type = checker_infer_let(check, (Ast_Let *)node, subst, ctx);
//			break;
//
//		case AST_FUN:
//			type = checker_infer_fun(check, (Ast_Fun *)node, subst, ctx);
//			break;
//
//		case AST_APP:
//			type = checker_infer_app(check, (Ast_App *)node, subst, ctx);
//			break;
//
//		case AST_IDENT:
//			type = checker_infer_ident(check, (Ast_Ident *)node, subst, ctx);
//			break;
//
//		case AST_UNARY:
//			type = checker_infer_unary(check, (Ast_Unary *)node, subst, ctx);
//			break;
//
//		case AST_BINARY:
//			type = checker_infer_binary(check, (Ast_Binary *)node, subst, ctx);
//			break;
//
//		case AST_TUPLE:
//			type = checker_infer_tuple(check, (Ast_Tuple *)node, subst, ctx);
//			break;
//
//		case AST_CONST:
//			type = consts[((Ast_Const *)node)->kind];
//			break;
//	}
//
//	node->type = type;
//	return type;
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
//MAC_HOT bool
//checker_infer_ast(Type_Checker *check, Ast_Top *ast, Error_List *err)
//{
//	check->err = err;
//	bool succ = true;
//
//	Type_Tab subst;
//	type_tab_init(&subst, 0);
//
//	for (size_t i = 0; i < ast->len; ++i)
//	{
//		Ast_Node *node = ast->items[i];
//		Type type = checker_infer(check, node, &subst, check->ctx);
//		node->type = checker_subst(check, type, &subst);
//
//		if (node->type == TYPE_NONE)
//		{
//			succ = false;
//		}
//		type_tab_reset(&subst);
//	}
//	return succ;
//}
//
//MAC_HOT bool
//checker_infer_node(Type_Checker *check, Ast_Node *node, Error_List *err)
//{
//	check->err = err;
//
//	Type_Tab subst;
//	type_tab_init(&subst, 0);
//
//	Type type = checker_infer(check, node, &subst, check->ctx);
//	node->type = checker_subst(check, type, &subst);
//	return node->type == TYPE_NONE;
//}
//
//
//
//
//
//MAC_CONST bool
//type_equal(Type t1, Type t2)
//{
//	const Type_Tag tag1 = TYPE_UNTAG(t1);
//	const Type_Tag tag2 = TYPE_UNTAG(t2);
//
//	if (tag1 == TAG_FUN && tag2 == TAG_FUN)
//	{
//		Type_Fun *fun1 = TYPE_PTR(t1);
//		Type_Fun *fun2 = TYPE_PTR(t2);
//
//		if (fun1->len != fun2->len)
//		{
//			return false;
//		}
//
//		for (size_t i = 0; i < fun1->len; ++i)
//		{
//			if (!type_equal(fun1->pars[i], fun2->pars[i]))
//			{
//				return false;
//			}
//		}
//		return type_equal(fun1->ret, fun2->ret);
//	}
//	else if (tag1 == TAG_TUPLE && tag2 == TAG_TUPLE)
//	{
//		Type_Tuple *tuple1 = TYPE_PTR(t1);
//		Type_Tuple *tuple2 = TYPE_PTR(t2);
//
//		if (tuple1->len != tuple2->len)
//		{
//			return false;
//		}
//
//		for (size_t i = 0; i < tuple1->len; ++i)
//		{
//			if (!type_equal(tuple1->items[i], tuple2->items[i]))
//			{
//				return false;
//			}
//		}
//		return true;
//	}
//	else if (tag1 == TAG_BIND && tag2 == TAG_BIND)
//	{
//		Type_Binder *bind1 = TYPE_PTR(t1);
//		Type_Binder *bind2 = TYPE_PTR(t2);
//
//		if (bind1->len != bind2->len)
//		{
//			return false;
//		}
//
//		for (size_t i = 0; i < bind1->len; ++i)
//		{
//			if (!type_equal(bind1->binds[i], bind2->binds[i]) ||
//				!STR_EQUAL(bind1->names[i], bind2->names[i]))
//			{
//				return false;
//			}
//		}
//		return type_equal(bind1->type, bind2->type);
//	}
//	else if (tag1 == TAG_VAR && tag2 == TAG_VAR)
//	{
//		return TYPE_TID(t1) == TYPE_TID(t2);
//	}
//
//	return tag1 == tag2;
//}
//
//
//
//-----------------------------------------------------
//		printf("_________________________\n");
//		constr_buf_iter(&constr, constr_buf_iter_dump, NULL);
//		printf("\n_________________________\n");
//		subst_tab_iter(&subst, subst_tab_iter_dump, NULL);
