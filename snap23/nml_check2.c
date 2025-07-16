#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "nml_check2.h"
#include "nml_const.h"
#include "nml_dbg.h"
#include "nml_err.h"
#include "nml_expr.h"
#include "nml_fmt.h"
#include "nml_mem.h"
#include "nml_patn.h"
#include "nml_set.h"
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
	Type_Id tid = check->tid++;
	//var->var = format_str(check->arena, STR_STATIC("T{u}"), tid);

	char tmp[100];
	int len = sprintf(tmp, "T%u", tid);
	char *tmp2 = arena_alloc(check->arena, len);
	memcpy(tmp2, tmp, len);
	var->var = STR_WLEN(tmp2, len);

	return TYPE_TAG(TAG_VAR, var);
}

static MAC_HOT void
checker_freevars(Type type, Str_Set *set)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		checker_freevars(fun->par, set);
		checker_freevars(fun->ret, set);
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		for (size_t i = 0; i < tuple->len; ++i)
		{
			checker_freevars(tuple->items[i], set);
		}
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		str_set_insert(set, var->var);
	}
}

static MAC_HOT void
checker_freevars_scheme(Type_Scheme scheme, Str_Set *set)
{
	for (size_t i = 0; i < scheme.len; ++i)
	{
		str_set_insert(set, scheme.vars[i]);
	}
}

static MAC_HOT void
checker_freevars_ctx(const Scheme_Tab *ctx, Str_Set *set)
{
	for (size_t i = 0; i < ctx->size; ++i)
	{
		Scheme_Tab_Buck *buck = ctx->bucks + i;
		if (buck->w_key)
		{
			checker_freevars_scheme(buck->v, set);
		}
	}
}

static MAC_HOT Type
checker_subst_rec(Type_Checker *check, Type type, const Type_Tab *tab, bool *subst)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		bool tmp = false;

		Type par = checker_subst_rec(check, fun->par, tab, &tmp);
		Type ret = checker_subst_rec(check, fun->ret, tab, &tmp);

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

		Type *items = arena_lock(check->arena, tuple->len * sizeof(Type));
		for (size_t i = 0; i < tuple->len; ++i)
		{
			items[i] = checker_subst_rec(check, tuple->items[i], tab, &tmp);
		}

		if (tmp)
		{
			arena_unlock(check->arena, items);
			return checker_tuple_new(check, items, tuple->len);
		}
		arena_update(check->arena, items, 0);
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		type_tab_get(tab, var->var, &type);
	}
	return type;
}

static MAC_INLINE inline Type
checker_subst(Type_Checker *check, Type type, const Type_Tab *tab)
{
	bool subst = false;
	return checker_subst_rec(check, type, tab, &subst);
}

static MAC_HOT inline Type_Scheme
checker_subst_scheme(Type_Checker *check, Type_Scheme scheme, const Type_Tab *tab)
{
	Type type;
	if (scheme.len != 0)
	{
		Type_Tab tmp;
		type_tab_init(&tmp, check->seed);
		type_tab_union(tab, &tmp);

		for (size_t i = 0; i < scheme.len; i++)
		{
			type_tab_remove(&tmp, scheme.vars[i]);
		}

		type = checker_subst(check, scheme.type, &tmp);
		type_tab_free(&tmp);
	}
	else
	{
		type = scheme.type;
	}

	return (Type_Scheme) {
		.type = type,
		.vars = scheme.vars,
		.len = scheme.len,
	};
}

static MAC_HOT void
checker_subst_ctx(Type_Checker *check, Scheme_Tab *ctx, const Type_Tab *tab)
{
	for (size_t i = 0; i < ctx->size; ++i)
	{
		Scheme_Tab_Buck *buck = ctx->bucks + i;
		if (buck->w_key)
		{
			buck->v = checker_subst_scheme(check, buck->v, tab);
		}
	}
}

static inline void
checker_compose(Type_Checker *check, const Type_Tab *tab, Type_Tab *dest)
{
	for (size_t i = 0; i < dest->size; ++i)
	{
		Type_Tab_Buck *buck = dest->bucks + i;
		if (buck->w_key)
		{
			buck->v = checker_subst(check, buck->v, tab);
		}
	}
	type_tab_union(tab, dest);
}

static MAC_HOT Type_Scheme
checker_generalize(Type_Checker *check, Type type, const Scheme_Tab *ctx)
{
	Str_Set set1;
	str_set_init(&set1, check->seed);
	checker_freevars(type, &set1);

	Str_Set set2;
	str_set_init(&set2, check->seed);
	checker_freevars_ctx(ctx, &set2);

	str_set_iter(&set1, str_set_iter_dump, NULL);
	str_set_iter(&set2, str_set_iter_dump, NULL);

	Str_Set diff;
	str_set_init(&diff, check->seed);
	str_set_difference(&set1, &set2, &diff);

	size_t len = 0;
	Sized_Str *vars = NULL;

	if (diff.len != 0)
	{
		for (size_t i = 0; i < diff.size; ++i)
		{
			if (diff.bucks->w_key)
			{
				++len;
			}
		}

		if (len != 0)
		{
			vars = arena_alloc(check->arena, len * sizeof(Sized_Str));
			for (size_t i = 0; i < diff.size; ++i)
			{
				if (diff.bucks->w_key)
				{
					vars[len--] = diff.bucks->hk.k;
				}
			}
		}
	}

	str_set_free(&set1);
	str_set_free(&set2);
	str_set_free(&diff);

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

	Type type = checker_subst(check, scheme.type, &tab);
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
	checker_freevars(type, &set);

	if (str_set_member(&set, name))
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Occurs check failed for `{S}` in type `{t}`"),
									name, type);
		checker_error(check, (Location) {0, 0, check->src}, msg);
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
		checker_unify(check, fun1->par, fun2->par, tab);

		Type ret1 = checker_subst(check, fun1->ret, tab);
		Type ret2 = checker_subst(check, fun2->ret, tab);
		checker_unify(check, ret1, ret2, tab);
	}
	else if (tag1 == TAG_TUPLE && tag2  == TAG_TUPLE)
	{
		Type_Tuple *tuple1 = TYPE_PTR(t1);
		Type_Tuple *tuple2 = TYPE_PTR(t2);

		if (true || tuple1->len != tuple2->len)
		{
			Sized_Str msg = format_str(check->arena,
										STR_STATIC("Unification of `{t}` with `{t}` failed"),
										t1, t2);
			checker_error(check, (Location) {0, 0, check->src}, msg);
			return;
		}

		for (size_t i = 0; i < tuple1->len; ++i)
		{
			checker_unify(check, tuple1->items[i], tuple2->items[i], tab);
		}
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
		checker_error(check, (Location) {0, 0, check->src}, msg);
	}
}

static MAC_HOT inline Type
checker_infer_const(Type_Checker *check, Const_Value value)
{
	MAC_UNUSED(check);

	const Type types[] = {
#define TAG(name, _, kind)	[CONST_ ## name] = TYPE_ ## name,
#define TYPE
#define BASE
#include "nml_tag.h"
#undef BASE
#undef TYPE
#undef TAG
 	};
	return types[value.kind];
}

static MAC_HOT inline Type
checker_infer_patn(Type_Checker *check, Scheme_Tab *ctx, Patn_Node *node, Type_Tab *tab)
{
	return TYPE_NONE;
}

static MAC_HOT inline Type
checker_infer_if(Type_Checker *check, Scheme_Tab *ctx, Expr_If *node, Type_Tab *tab)
{
	Type cond = checker_infer_expr(check, ctx, node->cond, tab);
	checker_unify(check, cond, TYPE_BOOL, tab);

	Type b_then = checker_infer_expr(check, ctx, node->b_then, tab);
	Type b_else = checker_infer_expr(check, ctx, node->b_else, tab);

	checker_unify(check, b_then, b_else, tab);
	return checker_subst(check, b_then, tab);
}

static MAC_HOT inline Type
checker_infer_match(Type_Checker *check, Scheme_Tab *ctx, Expr_Match *node, Type_Tab *tab)
{
	return TYPE_NONE;
}

static MAC_HOT inline Type
checker_infer_let(Type_Checker *check, Scheme_Tab *ctx, Expr_Let *node, Type_Tab *tab)
{
	Type value = checker_infer_expr(check, ctx, node->value, tab);
	if (node->hint != TYPE_NONE)
	{
		checker_unify(check, node->hint, value, tab);
	}

	Type_Scheme scheme;
	bool shad = scheme_tab_get(ctx, node->name, &scheme);
	scheme_tab_insert(ctx, node->name, checker_generalize(check, value, ctx));

	Type type = value;
	if (node->expr != NULL)
	{
		checker_subst_ctx(check, ctx, tab);
		type = checker_infer_expr(check, ctx, node->expr, tab);

		if (shad)
		{
			scheme_tab_insert(ctx, node->name, scheme);
		}
		else
		{
			scheme_tab_remove(ctx, node->name);
		}
	}
	return checker_subst(check, type, tab);
}

static MAC_HOT inline Type
checker_infer_fun(Type_Checker *check, Scheme_Tab *ctx, Expr_Fun *node, Type_Tab *tab)
{
	bool anon = STR_EQUAL(node->name, STR_STATIC("@anon"));
	bool shad = false;

	node->rec = checker_var_new(check);
	Type_Scheme scheme;

	if (!anon)
	{
		shad = scheme_tab_get(ctx, node->name, &scheme);
		scheme_tab_insert(ctx, node->name, checker_generalize(check, node->rec, ctx));
	}

	Scheme_Tab tmp;
	scheme_tab_init(&tmp, check->seed);
	scheme_tab_union(ctx, &tmp);

	Type pars[node->len];
	for (size_t i = 0; i < node->len; ++i)
	{
		pars[i] = checker_var_new(check);
		scheme_tab_insert(&tmp, node->pars[i], SCHEME_EMPTY(pars[i]));
	}

	node->fun = checker_infer_expr(check, &tmp, node->body, tab);
	scheme_tab_free(&tmp);

	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_subst(check, pars[i - 1], tab);
		node->fun = checker_fun_new(check, par, node->fun);
	}

	checker_unify(check, node->rec, node->fun, tab);
	if (node->hint != TYPE_NONE)
	{
		checker_unify(check, node->hint, node->fun, tab);
	}

	Type type = node->fun;
	if (node->expr != NULL)
	{
		type = checker_infer_expr(check, ctx, node->expr, tab);

		if (shad)
		{
			scheme_tab_insert(ctx, node->name, scheme);
		}
		else if (!anon)
		{
			scheme_tab_remove(ctx, node->name);
		}
	}
	return checker_subst(check, type, tab);
}

static MAC_HOT inline Type
checker_infer_app(Type_Checker *check, Scheme_Tab *ctx, Expr_App *node, Type_Tab *tab)
{
	if (node->fun->kind == EXPR_TUPLE || node->fun->kind == EXPR_CONST)
	{
		checker_error(check, node->fun->loc, STR_STATIC("Only functions can be applied"));
		return TYPE_NONE;
	}

	Type ret = checker_var_new(check);
	Type fun = ret;

	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_infer_expr(check, ctx, node->args[i - 1], tab);
		fun = checker_fun_new(check, par, fun);
	}

	checker_unify(check, fun, checker_infer_expr(check, ctx, node->fun, tab), tab);
	return checker_subst(check, ret, tab);
}

static MAC_HOT inline Type
checker_infer_ident(Type_Checker *check, Scheme_Tab *ctx, Expr_Ident *node, Type_Tab *tab)
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

	bool err;
	switch (node->op)
	{
#define TOK(kind, str, id, _)											\
	case TOK_ ## kind:													\
		err = !scheme_tab_get(check->ctx, STR_STATIC(str), &scheme);	\
		type = checker_instantiate(check, scheme);						\
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

	Type rhs = checker_infer_expr(check, ctx, node->rhs, tab);
	Type lhs = checker_infer_expr(check, ctx, node->lhs, tab);
	Type ret = checker_var_new(check);

	Type fun = checker_fun_new(check, rhs, checker_fun_new(check, lhs, ret));
	checker_unify(check, type, fun, tab);

//	Type_Fun *fun = TYPE_PTR(type);
//	Type rhs = checker_infer_expr(check, ctx, node->rhs, tab);
//	checker_unify(check, rhs, fun->par, tab);
//
//	fun = TYPE_PTR(fun->ret);
//	Type lhs = checker_infer_expr(check, ctx, node->lhs, tab);
//	checker_unify(check, lhs, fun->par, tab);
//
//	Type ret = checker_var_new(check);
//	checker_unify(check, ret, fun->ret, tab);
	return checker_subst(check, ret, tab);
}

static MAC_HOT inline Type
checker_infer_tuple(Type_Checker *check, Scheme_Tab *ctx, Expr_Tuple *node, Type_Tab *tab)
{
	Type *items = arena_alloc(check->arena, node->len * sizeof(Type));
	for (size_t i = 0; i < node->len; ++i)
	{
		items[i] = checker_infer_expr(check, ctx, node->items[i], tab);
		items[i] = checker_subst(check, items[i], tab);
	}

	return checker_tuple_new(check, items, node->len);
}

static MAC_HOT Type
checker_infer_expr(Type_Checker *check, Scheme_Tab *ctx, Expr_Node *node, Type_Tab *tab)
{
	Type type = TYPE_NONE;
	switch (node->kind)
	{
		case EXPR_IF:
			type = checker_infer_if(check, ctx, (Expr_If *)node, tab);
			break;

		case EXPR_MATCH:
			type = checker_infer_match(check, ctx, (Expr_Match *)node, tab);
			break;

		case EXPR_LET:
			type = checker_infer_let(check, ctx, (Expr_Let *)node, tab);
			break;

		case EXPR_FUN:
			type = checker_infer_fun(check, ctx, (Expr_Fun *)node, tab);
			break;

		case EXPR_APP:
			type = checker_infer_app(check, ctx, (Expr_App *)node, tab);
			break;

		case EXPR_IDENT:
			type = checker_infer_ident(check, ctx, (Expr_Ident *)node, tab);
			break;

		case EXPR_BINARY:
			type = checker_infer_binary(check, ctx, (Expr_Binary *)node, tab);
			break;

		case EXPR_TUPLE:
			type = checker_infer_tuple(check, ctx, (Expr_Tuple *)node, tab);
			break;

		case EXPR_CONST:
			type = checker_infer_const(check, ((Expr_Const *)node)->value);
			break;
	}

	node->type = type;
	return type;
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
		if (MAC_LIKELY(ast->items[i].kind != AST_SIG))
		{
			Expr_Node *node = ast->items[i].node;
			Type type = checker_infer_expr(check, check->ctx, node, &tab);

			if (MAC_UNLIKELY(!check->succ))
			{
				check->succ = true;
				succ = false;
				continue;
			}

			if (node->kind == EXPR_FUN)
			{
				Expr_Fun *fun = (Expr_Fun *)node;
				if (fun->expr == NULL && !STR_EQUAL(fun->name, STR_STATIC("@anon")))
				{
					scheme_tab_insert(check->ctx, fun->name, checker_generalize(check, type, check->ctx));
				}
			}
			else if (node->kind == EXPR_LET)
			{
				Expr_Let *let = (Expr_Let *)node;
				if (let->expr == NULL)
				{
					scheme_tab_insert(check->ctx, let->name, checker_generalize(check, type, check->ctx));
				}
			}
		}
	}

	type_tab_free(&tab);
	return succ;
}
