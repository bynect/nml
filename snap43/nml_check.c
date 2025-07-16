#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "nml_buf.h"
#include "nml_const.h"
#include "nml_dbg.h"
#include "nml_err.h"
#include "nml_expr.h"
#include "nml_fmt.h"
#include "nml_mem.h"
#include "nml_patn.h"
#include "nml_set.h"
#include "nml_decl.h"
#include "nml_str.h"
#include "nml_type.h"
#include "nml_check.h"
#include "nml_ast.h"
#include "nml_mac.h"

static MAC_HOT Type checker_infer_expr(Type_Checker *check, Scheme_Tab *ctx, Expr_Node *node, Type_Tab *tab);

static MAC_INLINE inline void
checker_error(Type_Checker *check, Location loc, Sized_Str msg)
{
	error_append(check->err, check->arena, msg, loc, ERR_ERROR);
	check->succ = false;
}

static MAC_INLINE inline Type
checker_fun_new(Type_Checker *check, Type par, Type ret)
{
	Type_Fun *fun = arena_alloc(check->arena, sizeof(Type_Fun));
	fun->par = par;
	fun->ret = ret;
	return TYPE_TAG(TAG_FUN, fun);
}

static MAC_INLINE inline Type
checker_tuple_new(Type_Checker *check, Type *items, size_t len)
{
	Type_Tuple *tuple = arena_alloc(check->arena, sizeof(Type_Tuple));
	tuple->items = items;
	tuple->len = len;
	return TYPE_TAG(TAG_TUPLE, tuple);
}

static MAC_INLINE inline Type
checker_var_new(Type_Checker *check)
{
	Type_Var *var = arena_alloc(check->arena, sizeof(Type_Var));
	var->var = format_str(check->arena, STR_STATIC("T{u}"), check->tid++);
	return TYPE_TAG(TAG_VAR, var);
}

static MAC_HOT void
checker_ftv_type(Type type, Str_Set *set)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		checker_ftv_type(fun->par, set);
		checker_ftv_type(fun->ret, set);
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		for (size_t i = 0; i < tuple->len; ++i)
		{
			checker_ftv_type(tuple->items[i], set);
		}
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		str_set_insert(set, var->var);
	}
}

static MAC_HOT Type
checker_apply_rec(Type_Checker *check, Type type, const Type_Tab *tab, bool *subst)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		bool tmp = false;

		Type par = checker_apply_rec(check, fun->par, tab, &tmp);
		Type ret = checker_apply_rec(check, fun->ret, tab, &tmp);

		if (tmp)
		{
			*subst = true;
			return checker_fun_new(check, par, ret);
		}
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		bool tmp = false;

		Type items[tuple->len];
		for (size_t i = 0; i < tuple->len; ++i)
		{
			items[i] = checker_apply_rec(check, tuple->items[i], tab, &tmp);
		}

		if (tmp)
		{
			Type *copy = arena_alloc(check->arena, tuple->len * sizeof(Type));
			memcpy(copy, items, tuple->len * sizeof(Type));

			*subst = true;
			return checker_tuple_new(check, copy, tuple->len);
		}
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		if (type_tab_get(tab, var->var, &type))
		{
			*subst = true;
		}
	}
	return type;
}

static MAC_INLINE inline Type
checker_apply_type(Type_Checker *check, Type type, const Type_Tab *tab)
{
	bool subst = false;
	return checker_apply_rec(check, type, tab, &subst);
}

static MAC_HOT void
checker_ftv_scheme(Type_Scheme scheme, Str_Set *set)
{
	checker_ftv_type(scheme.type, set);
	for (size_t i = 0; i < scheme.len; ++i)
	{
		str_set_remove(set, scheme.vars[i]);
	}
}

static MAC_HOT inline Type_Scheme
checker_apply_scheme(Type_Checker *check, Type_Scheme scheme, const Type_Tab *tab)
{
	Type type = scheme.type;
	if (scheme.len != 0)
	{
		Type_Tab tmp;
		type_tab_init(&tmp, check->seed);
		type_tab_union(tab, &tmp);

		for (size_t i = 0; i < scheme.len; i++)
		{
			type_tab_remove(&tmp, scheme.vars[i]);
		}

		type = checker_apply_type(check, type, &tmp);
		type_tab_free(&tmp);
	}

	return (Type_Scheme) {
		.type = type,
		.vars = scheme.vars,
		.len = scheme.len,
	};
}

static MAC_HOT void
checker_ftv_ctx(const Scheme_Tab *ctx, Str_Set *set)
{
	for (size_t i = 0; i < ctx->size; ++i)
	{
		Scheme_Tab_Buck *buck = ctx->bucks + i;
		if (buck->set)
		{
			checker_ftv_scheme(buck->v, set);
		}
	}
}

static MAC_HOT void
checker_apply_ctx(Type_Checker *check, Scheme_Tab *ctx, const Type_Tab *tab)
{
	for (size_t i = 0; i < ctx->size; ++i)
	{
		Scheme_Tab_Buck *buck = ctx->bucks + i;
		if (buck->set)
		{
			buck->v = checker_apply_scheme(check, buck->v, tab);
		}
	}
}

static MAC_HOT void
checker_compose(Type_Checker *check, const Type_Tab *MAC_RESTRICT s1, Type_Tab *MAC_RESTRICT s2)
{
	for (size_t i = 0; i < s2->size; ++i)
	{
		Type_Tab_Buck *buck = s2->bucks + i;
		if (buck->set)
		{
			buck->v = checker_apply_type(check, buck->v, s1);
		}
	}

	for (size_t i = 0; i < s1->size; ++i)
	{
		Type_Tab_Buck *buck = s1->bucks + i;
		if (buck->set)
		{
			type_tab_insert_key(s2, buck->hk, buck->v);
		}
	}
}

static MAC_HOT Type_Scheme
checker_generalize(Type_Checker *check, Type type, const Scheme_Tab *ctx)
{
	Str_Set set1;
	str_set_init(&set1, check->seed);
	checker_ftv_type(type, &set1);

	Str_Set set2;
	str_set_init(&set2, check->seed);
	checker_ftv_ctx(ctx, &set2);

	size_t len = set1.len;
	Sized_Str *vars = NULL;

	// NOTE: Set difference optimization
	for (size_t i = 0; i < set2.size; ++i)
	{
		Str_Set_Buck *buck = set2.bucks + i;
		if (buck->set)
		{
			if (str_set_remove_key(&set1, buck->hk))
			{
				--len;
			}
		}
	}

	if (len != 0)
	{
		vars = arena_alloc(check->arena, len * sizeof(Sized_Str));
		for (size_t i = 0, j = 0; i < set1.size; ++i)
		{
			Str_Set_Buck *buck = set1.bucks + i;
			if (buck->set)
			{
				vars[j++] = buck->hk.k;
			}
		}
	}

	str_set_free(&set1);
	str_set_free(&set2);

	return (Type_Scheme) {
		.type = type,
		.vars = vars,
		.len = len,
	};
}

static MAC_HOT Type
checker_instantiate(Type_Checker *check, Type_Scheme scheme)
{
	if (scheme.len == 0)
	{
		return scheme.type;
	}

	Type_Tab tab;
	type_tab_init(&tab, check->seed);

	for (size_t i = 0; i < scheme.len; ++i)
	{
		type_tab_insert(&tab, scheme.vars[i], checker_var_new(check));
	}

	Type type = checker_apply_type(check, scheme.type, &tab);
	type_tab_free(&tab);
	return type;
}

static MAC_INLINE inline void
checker_bind(Type_Checker *check, Type type, Sized_Str name, Type_Tab *tab)
{
	if (TYPE_UNTAG(type) == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		if (STR_EQUAL(var->var, name))
		{
			return;
		}
	}

	Str_Set set;
	str_set_init(&set, check->seed);
	checker_ftv_type(type, &set);

	if (str_set_member(&set, name))
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Occurs check failed for `{S}` in type `{t}`"),
									name, type);
		checker_error(check, location_new(0, 0, check->src), msg);
	}
	else
	{
		type_tab_insert(tab, name, type);
	}
	str_set_free(&set);
}

static MAC_HOT void
checker_unify(Type_Checker *check, Type t1, Type t2, Type_Tab *tab)
{
	const Type_Tag tag1 = TYPE_UNTAG(t1);
	const Type_Tag tag2 = TYPE_UNTAG(t2);

	if (tag1 == TAG_FUN && tag2 == TAG_FUN)
	{
		Type_Fun *fun1 = TYPE_PTR(t1);
		Type_Fun *fun2 = TYPE_PTR(t2);

		Type_Tab tmp1;
		type_tab_init(&tmp1, check->seed);
		checker_unify(check, fun1->par, fun2->par, &tmp1);

		Type ret1 = checker_apply_type(check, fun1->ret, &tmp1);
		Type ret2 = checker_apply_type(check, fun2->ret, &tmp1);

		Type_Tab tmp2;
		type_tab_init(&tmp2, check->seed);
		checker_unify(check, ret1, ret2, &tmp2);

		checker_compose(check, &tmp2, &tmp1);
		checker_compose(check, &tmp1, tab);

		type_tab_free(&tmp1);
		type_tab_free(&tmp2);
	}
	else if (tag1 == TAG_TUPLE && tag2  == TAG_TUPLE)
	{
		Type_Tuple *tuple1 = TYPE_PTR(t1);
		Type_Tuple *tuple2 = TYPE_PTR(t2);

		if (tuple1->len != tuple2->len)
		{
			Sized_Str msg = format_str(check->arena,
										STR_STATIC("Unification of `{t}` with `{t}` failed"),
										t1, t2);
			checker_error(check, location_new(0, 0, check->src), msg);
			return;
		}

		Type_Tab tmp1;
		type_tab_init(&tmp1, check->seed);

		Type_Tab tmp2;
		type_tab_init(&tmp2, check->seed);

		for (size_t i = 0; i < tuple1->len; ++i)
		{
			Type item1 = checker_apply_type(check, tuple1->items[i], &tmp1);
			Type item2 = checker_apply_type(check, tuple2->items[i], &tmp1);

			checker_unify(check, item1, item2, &tmp2);
			checker_compose(check, &tmp2, &tmp1);
			type_tab_reset(&tmp2);
		}

		checker_compose(check, &tmp1, tab);
		type_tab_free(&tmp1);
		type_tab_free(&tmp2);
	}
	else if (tag1 == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(t1);
		checker_bind(check, t2, var->var, tab);
	}
	else if (tag2 == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(t2);
		checker_bind(check, t1, var->var, tab);
	}
	else if (tag1 != tag2)
	{
		Sized_Str msg = format_str(check->arena,
								STR_STATIC("Unification of `{t}` with `{t}` failed"),
								t1, t2);
		checker_error(check, location_new(0, 0, check->src), msg);
	}
}

static MAC_HOT inline Type
checker_infer_const(Const_Value value)
{
	const Type types[] = {
#define TAG(name, _, kind)	[CONST_ ## name] = TYPE_ ## name,
#define TYPE
#define PRIM
#include "nml_tag.h"
#undef PRIM
#undef TYPE
#undef TAG
 	};
	return types[value.kind];
}

static MAC_HOT Type
checker_infer_patn(Type_Checker *check, Scheme_Tab *ctx, Patn_Node *node, Type_Tab *tab)
{
	Type type = TYPE_NONE;
	if (node->kind == PATN_IDENT)
	{
		Patn_Ident *ident = (Patn_Ident *)node;
		type = checker_var_new(check);
		scheme_tab_insert(ctx, ident->name, SCHEME_EMPTY(type));
	}
	else if (node->kind == PATN_CONSTR)
	{
		// TODO
		checker_error(check, node->loc, STR_STATIC("Data constructors not implemented"));
	}
	else if (node->kind == PATN_AS)
	{
		Patn_As *as = (Patn_As *)node;
		type = checker_infer_patn(check, ctx, as->patn, tab);
		scheme_tab_insert(ctx, as->name, SCHEME_EMPTY(type));
	}
	else if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		Type *items = arena_alloc(check->arena, tuple->len * sizeof(Type));

		for (size_t i = 0; i < tuple->len; ++i)
		{
			items[i] = checker_infer_patn(check, ctx, tuple->items[i], tab);
		}
		type = checker_tuple_new(check, items, tuple->len);
	}
	else
	{
		type = checker_infer_const(((Expr_Const *)node)->value);
	}

	node->type = type;
	return type;
}

static MAC_HOT inline Type
checker_infer_if(Type_Checker *check, Scheme_Tab *ctx, Expr_If *node, Type_Tab *tab)
{
	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);

	Type cond = checker_infer_expr(check, ctx, node->cond, &tmp1);
	checker_unify(check, checker_apply_type(check, cond, &tmp1), TYPE_BOOL, &tmp1);

	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(ctx, &scope);
	checker_apply_ctx(check, &scope, &tmp1);

	Type_Tab tmp2;
	type_tab_init(&tmp2, check->seed);

	Type b_then = checker_infer_expr(check, &scope, node->b_then, &tmp2);
	Type b_else = checker_infer_expr(check, &scope, node->b_else, &tmp2);
	scheme_tab_free(&scope);

	Type_Tab tmp3;
	type_tab_init(&tmp3, check->seed);
	checker_unify(check, checker_apply_type(check, b_then, &tmp2),
					checker_apply_type(check, b_else, &tmp2), &tmp3);

	checker_compose(check, &tmp3, &tmp2);
	checker_compose(check, &tmp2, &tmp1);
	checker_compose(check, &tmp1, tab);

	type_tab_free(&tmp1);
	type_tab_free(&tmp2);
	type_tab_free(&tmp3);
	return b_then;
}

static MAC_HOT inline Type
checker_infer_match(Type_Checker *check, Scheme_Tab *ctx, Expr_Match *node, Type_Tab *tab)
{
	Type value = checker_infer_expr(check, ctx, node->value, tab);
	Type type = checker_var_new(check);

	if (!patn_match_check(node, check->arena, &check->set, check->err))
	{
		check->succ = false;
	}

	Scheme_Tab tmp;
	scheme_tab_init(&tmp, check->seed);

	for (size_t i = 0; i < node->len; ++i)
	{
		scheme_tab_union(ctx, &tmp);
		Type arm = checker_infer_patn(check, &tmp, node->arms[i].patn, tab);
		checker_unify(check, value, arm, tab);

		arm = checker_infer_expr(check, &tmp, node->arms[i].expr, tab);
		checker_unify(check, type, arm, tab);
		scheme_tab_reset(&tmp);
	}

	scheme_tab_free(&tmp);
	return type;
}

static MAC_HOT inline Type
checker_infer_let(Type_Checker *check, Scheme_Tab *ctx, Expr_Let *node, Type_Tab *tab)
{
	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);

	Type value = checker_infer_expr(check, ctx, node->value, &tmp1);
	if (node->hint != TYPE_NONE)
	{
		checker_unify(check, node->hint, checker_apply_type(check, value, &tmp1), &tmp1);
	}

	if (!STR_EQUAL(node->name, STR_STATIC("_")))
	{
		scheme_tab_remove(ctx, node->name);
		scheme_tab_insert(ctx, node->name, checker_generalize(check, value, ctx));
	}

	Type_Tab tmp2;
	type_tab_init(&tmp2, check->seed);

	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(ctx, &scope);

	scheme_tab_remove(&scope, node->name);
	checker_apply_ctx(check, &scope, &tmp1);
	scheme_tab_insert(&scope, node->name, checker_generalize(check, value, &scope));

	Type type = checker_infer_expr(check, &scope, node->expr, &tmp2);
	checker_compose(check, &tmp2, &tmp1);
	checker_compose(check, &tmp1, tab);

	type_tab_free(&tmp1);
	type_tab_free(&tmp2);
	scheme_tab_free(&scope);
	return type;
}

static MAC_HOT inline Type
checker_infer_fun(Type_Checker *check, Scheme_Tab *ctx, Expr_Fun *node, Type_Tab *tab)
{
	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(ctx, &scope);

	Type rec = checker_var_new(check);
	scheme_tab_insert(&scope, node->name, SCHEME_EMPTY(rec));

	Type pars[node->len];
	for (size_t i = 0; i < node->len; ++i)
	{
		pars[i] = checker_var_new(check);
		scheme_tab_insert(&scope, node->pars[i], SCHEME_EMPTY(pars[i]));
	}

	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);
	Type ret = checker_infer_expr(check, &scope, node->body, &tmp1);

	node->fun = ret;
	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_apply_type(check, pars[i - 1], &tmp1);
		node->fun = checker_fun_new(check, par, node->fun);
	}

	Type_Tab tmp2;
	type_tab_init(&tmp2, check->seed);

	checker_unify(check, rec, node->fun, &tmp2);
	if (node->hint != TYPE_NONE)
	{
		checker_unify(check, node->hint, node->fun, &tmp2);
	}

	scheme_tab_remove(ctx, node->name);
	checker_apply_ctx(check, ctx, &tmp1);
	scheme_tab_insert(ctx, node->name, checker_generalize(check, node->fun, ctx));

	scheme_tab_reset(&scope);
	scheme_tab_union(ctx, &scope);

	Type_Tab tmp3;
	type_tab_init(&tmp3, check->seed);
	Type type = checker_infer_expr(check, &scope, node->expr, &tmp3);

	checker_compose(check, &tmp3, &tmp2);
	checker_compose(check, &tmp2, &tmp1);
	checker_compose(check, &tmp1, tab);

	type_tab_free(&tmp1);
	type_tab_free(&tmp2);
	type_tab_free(&tmp3);
	scheme_tab_free(&scope);
	return type;
}

static MAC_HOT inline Type
checker_infer_lambda(Type_Checker *check, Scheme_Tab *ctx, Expr_Lambda *node, Type_Tab *tab)
{
	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(ctx, &scope);

	Type pars[node->len];
	for (size_t i = 0; i < node->len; ++i)
	{
		pars[i] = checker_var_new(check);
		scheme_tab_insert(&scope, node->pars[i], SCHEME_EMPTY(pars[i]));
	}

	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);
	Type ret = checker_infer_expr(check, &scope, node->body, &tmp1);

	Type fun = ret;
	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_apply_type(check, pars[i - 1], &tmp1);
		fun = checker_fun_new(check, par, fun);
	}
	checker_compose(check, &tmp1, tab);

	type_tab_free(&tmp1);
	scheme_tab_free(&scope);
	return fun;
}

static MAC_HOT inline Type
checker_infer_app(Type_Checker *check, Scheme_Tab *ctx, Expr_App *node, Type_Tab *tab)
{
	if (node->fun->kind == EXPR_TUPLE || node->fun->kind == EXPR_CONST)
	{
		checker_error(check, node->fun->loc, STR_STATIC("Only functions can be applied"));
		return TYPE_NONE;
	}

	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);

	Type fun1 = checker_infer_expr(check, ctx, node->fun, &tmp1);
	Type ret = checker_var_new(check);

	Type_Tab tmp2;
	type_tab_init(&tmp2, check->seed);

	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(ctx, &scope);

	Type fun2 = ret;
	for (size_t i = node->len; i > 0; --i)
	{
		checker_apply_ctx(check, &scope, &tmp1);
		Type par = checker_infer_expr(check, &scope, node->args[i - 1], &tmp2);
		fun2 = checker_fun_new(check, par, fun2);

		checker_compose(check, &tmp2, &tmp1);
		type_tab_reset(&tmp2);
	}
	scheme_tab_free(&scope);

	Type_Tab tmp3;
	type_tab_init(&tmp3, check->seed);
	checker_unify(check, checker_apply_type(check, fun1, &tmp1), fun2, &tmp3);
	ret = checker_apply_type(check, ret, &tmp3);

	checker_compose(check, &tmp3, &tmp1);
	checker_compose(check, &tmp1, tab);

	type_tab_free(&tmp1);
	type_tab_free(&tmp2);
	type_tab_free(&tmp3);
	return ret;
}

static MAC_HOT inline Type
checker_infer_ident(Type_Checker *check, Scheme_Tab *ctx, Expr_Ident *node)
{
	Type_Scheme scheme;
	if (!scheme_tab_get(ctx, node->name, &scheme))
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Unbound identifier `{S}`"),
									node->name);
		checker_error(check, node->node.loc, msg);
		return TYPE_NONE;
	}

	return checker_instantiate(check, scheme);
}

static MAC_HOT inline Type
checker_infer_binary(Type_Checker *check, Scheme_Tab *ctx, Expr_Binary *node, Type_Tab *tab)
{
	Type_Scheme scheme;
	Type type;

	bool err = false;
	switch (node->op)
	{
#define TOK(kind, str, id, _)											\
	case TOK_ ## kind:													\
		if (scheme_tab_get(check->ctx, STR_STATIC(str), &scheme))		\
		{																\
			type = checker_instantiate(check, scheme);					\
		}																\
		else															\
		{																\
			err = true;													\
		}																\
		break;
#define OPER
#include "nml_tok.h"
#undef OPER
#undef TOK

		default:
			err = true;
			break;
	}

	if (err || TYPE_UNTAG(type) != TAG_FUN)
	{
		Sized_Str msg = format_str(check->arena,
								STR_STATIC("Invalid binary operator `{K}`"), node->op);
		checker_error(check, node->node.loc, msg);
		return TYPE_NONE;
	}

	Type ret = checker_var_new(check);
	Type rhs = checker_infer_expr(check, ctx, node->rhs, tab);
	Type lhs = checker_infer_expr(check, ctx, node->lhs, tab);

	checker_unify(check, type, checker_fun_new(check, rhs, checker_fun_new(check, lhs, ret)), tab);
	return ret;
}

static MAC_HOT inline Type
checker_infer_tuple(Type_Checker *check, Scheme_Tab *ctx, Expr_Tuple *node, Type_Tab *tab)
{
	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);

	Type_Tab tmp2;
	type_tab_init(&tmp2, check->seed);

	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(ctx, &scope);

	Type *items = arena_alloc(check->arena, node->len * sizeof(Type));
	for (size_t i = 0; i < node->len; ++i)
	{
		checker_apply_ctx(check, &scope, &tmp1);
		items[i] = checker_infer_expr(check, ctx, node->items[i], &tmp2);

		checker_compose(check, &tmp2, &tmp1);
		type_tab_reset(&tmp2);
	}

	checker_compose(check, &tmp1, tab);
	type_tab_free(&tmp1);
	type_tab_free(&tmp2);

	scheme_tab_free(&scope);
	return checker_tuple_new(check, items, node->len);
}

static MAC_HOT Type
checker_infer_expr(Type_Checker *check, Scheme_Tab *ctx, Expr_Node *node, Type_Tab *tab)
{
	switch (node->kind)
	{
		case EXPR_IF:
			node->type = checker_infer_if(check, ctx, (Expr_If *)node, tab);
			break;

		case EXPR_MATCH:
			node->type = checker_infer_match(check, ctx, (Expr_Match *)node, tab);
			break;

		case EXPR_LET:
			node->type = checker_infer_let(check, ctx, (Expr_Let *)node, tab);
			break;

		case EXPR_FUN:
			node->type = checker_infer_fun(check, ctx, (Expr_Fun *)node, tab);
			break;

		case EXPR_LAMBDA:
			node->type = checker_infer_lambda(check, ctx, (Expr_Lambda *)node, tab);
			break;

		case EXPR_APP:
			node->type = checker_infer_app(check, ctx, (Expr_App *)node, tab);
			break;

		case EXPR_IDENT:
			node->type = checker_infer_ident(check, ctx, (Expr_Ident *)node);
			break;

		case EXPR_BINARY:
			node->type = checker_infer_binary(check, ctx, (Expr_Binary *)node, tab);
			break;

		case EXPR_TUPLE:
			node->type = checker_infer_tuple(check, ctx, (Expr_Tuple *)node, tab);
			break;

		case EXPR_CONST:
			node->type = checker_infer_const(((Expr_Const *)node)->value);
			break;
	}
	return node->type;
}

static MAC_HOT void
checker_apply_patn(Type_Checker *check, Patn_Node *node, const Type_Tab *tab)
{
	node->type = checker_apply_type(check, node->type, tab);
	if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;

		for (size_t i = 0; i < tuple->len; ++i)
		{
			checker_apply_patn(check, tuple->items[i], tab);
		}
	}
}

static MAC_HOT void
checker_apply_expr(Type_Checker *check, Expr_Node *node, const Type_Tab *tab)
{
	node->type = checker_apply_type(check, node->type, tab);
	switch (node->kind)
	{
		case EXPR_IF:
			checker_apply_expr(check, MAC_CAST(Expr_If *, node)->cond, tab);
			checker_apply_expr(check, MAC_CAST(Expr_If *, node)->b_then, tab);
			checker_apply_expr(check, MAC_CAST(Expr_If *, node)->b_else, tab);
			break;

		case EXPR_MATCH:
			checker_apply_expr(check, MAC_CAST(Expr_Match *, node)->value, tab);

			for (size_t i = 0; i < MAC_CAST(Expr_Match *, node)->len; ++i)
			{
				checker_apply_patn(check, MAC_CAST(Expr_Match *, node)->arms[i].patn, tab);
				checker_apply_expr(check, MAC_CAST(Expr_Match *, node)->arms[i].expr, tab);
			}
			break;

		case EXPR_LET:
			checker_apply_expr(check, MAC_CAST(Expr_Let *, node)->value, tab);
			checker_apply_expr(check, MAC_CAST(Expr_Let *, node)->expr, tab);
			break;

		case EXPR_FUN:
			checker_apply_expr(check, MAC_CAST(Expr_Fun *, node)->body, tab);
			checker_apply_expr(check, MAC_CAST(Expr_Fun *, node)->expr, tab);
			break;

		case EXPR_LAMBDA:
			checker_apply_expr(check, MAC_CAST(Expr_Lambda *, node)->body, tab);
			break;

		case EXPR_APP:
			checker_apply_expr(check, MAC_CAST(Expr_App *, node)->fun, tab);
			for (size_t i = 0; i < MAC_CAST(Expr_App *, node)->len; ++i)
			{
				checker_apply_expr(check, MAC_CAST(Expr_App *, node)->args[i], tab);
			}
			break;

		case EXPR_IDENT:
			break;

		case EXPR_BINARY:
			checker_apply_expr(check, MAC_CAST(Expr_Binary *, node)->rhs, tab);
			checker_apply_expr(check, MAC_CAST(Expr_Binary *, node)->lhs, tab);
			break;

		case EXPR_TUPLE:
			for (size_t i = 0; i < MAC_CAST(Expr_Tuple *, node)->len; ++i)
			{
				checker_apply_expr(check, MAC_CAST(Expr_Tuple *, node)->items[i], tab);
			}
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_INLINE inline void
checker_decl_let(Type_Checker *check, Decl_Let *node, Type_Tab *tab)
{
	checker_infer_expr(check, check->ctx, node->value, tab);
	if (node->hint != TYPE_NONE)
	{
		checker_unify(check, node->hint, checker_apply_type(check, node->value->type, tab), tab);
	}

	checker_apply_expr(check, node->value, tab);
	scheme_tab_insert(check->ctx, node->name, checker_generalize(check, node->value->type, check->ctx));
}

static MAC_INLINE inline void
checker_decl_fun(Type_Checker *check, Decl_Fun *node, Type_Tab *tab)
{
	Scheme_Tab scope;
	scheme_tab_init(&scope, check->seed);
	scheme_tab_union(check->ctx, &scope);

	Type rec = checker_var_new(check);
	scheme_tab_insert(&scope, node->name, SCHEME_EMPTY(rec));

	Type pars[node->len];
	for (size_t i = 0; i < node->len; ++i)
	{
		pars[i] = checker_var_new(check);
		scheme_tab_insert(&scope, node->pars[i], SCHEME_EMPTY(pars[i]));
	}

	Type_Tab tmp1;
	type_tab_init(&tmp1, check->seed);
	Type ret = checker_infer_expr(check, &scope, node->body, &tmp1);

	node->fun = ret;
	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_apply_type(check, pars[i - 1], &tmp1);
		node->fun = checker_fun_new(check, par, node->fun);
	}

	Type_Tab tmp2;
	type_tab_init(&tmp2, check->seed);

	checker_unify(check, rec, node->fun, &tmp2);
	if (node->hint != TYPE_NONE)
	{
		checker_unify(check, node->hint, node->fun, &tmp2);
	}

	checker_compose(check, &tmp2, &tmp1);
	checker_compose(check, &tmp1, tab);

	type_tab_free(&tmp1);
	type_tab_free(&tmp2);
	scheme_tab_free(&scope);

	checker_apply_expr(check, node->body, tab);
	scheme_tab_insert(check->ctx, node->name, checker_generalize(check, node->fun, check->ctx));

}

static MAC_INLINE inline void
checker_decl_data(Type_Checker *check, Decl_Data *node, Type_Tab *tab)
{
	str_set_reset(&check->set);
	for (size_t i = 0; i < node->len; ++i)
	{
		if (str_set_unique(&check->set, node->constrs[i].name) & SET_FAIL)
		{
			Sized_Str msg = format_str(check->arena,
										STR_STATIC("Constructor `{S}` used multiple times"),
										node->constrs[i].name);
			checker_error(check, node->node.loc, msg);
		}
	}

	// TODO
	checker_error(check, node->node.loc, STR_STATIC("Data types not implemented"));
}

static MAC_INLINE inline void
checker_decl_type(Type_Checker *check, Decl_Type *node, Type_Tab *tab)
{
	node->type = checker_apply_type(check, node->type, tab);
	type_tab_insert(tab, node->name, node->type);
}

static void
checker_infer_decl(Type_Checker *check, Decl_Node *node, Type_Tab *tab)
{
	switch (node->kind)
	{
		case DECL_LET:
			checker_decl_let(check, (Decl_Let *)node, tab);
			break;

		case DECL_FUN:
			checker_decl_fun(check, (Decl_Fun *)node, tab);
			break;

		case DECL_DATA:
			checker_decl_data(check, (Decl_Data *)node, tab);
			break;

		case DECL_TYPE:
			checker_decl_type(check, (Decl_Type *)node, tab);
			break;
	}
}

void
checker_init(Type_Checker *check, Hash_Default seed, Arena *arena, const Source *src, Scheme_Tab *ctx)
{
	check->seed = seed;
	check->tid = 0;
	check->arena = arena;

	check->err = NULL;
	check->succ = true;

	check->src = src;
	check->ctx = ctx;

	str_set_init(&check->set, seed);
}

void
checker_free(Type_Checker *check)
{
	str_set_free(&check->set);
}

MAC_HOT bool
checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err)
{
	check->err = err;
	bool succ = true;

	Type_Tab tab;
	type_tab_init(&tab, check->seed);

	for (size_t i = 0; i < ast->len; ++i)
	{
		Decl_Node *node = ast->decls[i];
		checker_infer_decl(check, node, &tab);

		if (MAC_UNLIKELY(!check->succ))
		{
			check->succ = true;
			succ = false;
			continue;
		}
	}

	type_tab_free(&tab);
	return succ;
}
