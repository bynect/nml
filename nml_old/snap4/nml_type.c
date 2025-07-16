#include "nml_type.h"
#include "nml_ast.h"
#include "nml_err.h"
#include "nml_fmt.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"

static MAC_HOT Type checker_infer_node(Type_Checker *check, Scheme_Tab *ctx,
										Ast_Node *node, Subst_Tab *subst);

static MAC_INLINE inline void
checker_error(Type_Checker *check, Sized_Str msg)
{
	error_append(check->err, check->arena, msg, 0, ERR_ERROR);
	printf("%zu\n", msg.len);
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
	var->name = format_str(check->arena, STR_STATIC("V{u}"), check->tid++);
	return TYPE_TAG(TAG_VAR, var);
}

static MAC_INLINE inline Type_Scheme
checker_scheme_new(Type_Checker *check, Sized_Str *vars, size_t len, Type type)
{
	return (Type_Scheme) {
		.vars = vars,
		.len = len,
		.type = type,
	};
}

static MAC_HOT Type
checker_substitute(Type_Checker *check, Type type, const Subst_Tab *subst)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		return checker_fun_new(check,
				checker_substitute(check, fun->par, subst),
				checker_substitute(check, fun->ret, subst));
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		Type *items = arena_alloc(check->arena, tuple->len * sizeof(Type));

		for (size_t i = 0; i < tuple->len; ++i)
		{
			items[i] = checker_substitute(check, tuple->items[i], subst);
		}
		return checker_tuple_new(check, items, tuple->len);
	}
	else if (tag == TAG_VAR)
	{
		Type_Var *var = TYPE_PTR(type);
		subst_tab_get(subst, var->name, &type);
	}

	return type;
}

static MAC_HOT void
checker_substitute_scheme(Type_Checker *check, Type_Scheme *scheme, const Subst_Tab *subst)
{
	Subst_Tab tmp;
	subst_tab_init(&tmp, check->seed);
	subst_tab_merge(subst, &tmp, check->arena);

	for (size_t i = scheme->len; i > 0; --i)
	{
		subst_tab_del(&tmp, scheme->vars[i]);
	}

	scheme->type = checker_substitute(check, scheme->type, &tmp);
}

static MAC_HOT void
checker_substitute_ctx(Type_Checker *check, Scheme_Tab *ctx, const Subst_Tab *subst)
{
	for (size_t i = 0; i < ctx->size; ++i)
	{
		Scheme_Tab_Buck *buck = ctx->bucks + i;
		if (buck->w_key)
		{
			checker_substitute_scheme(check, &buck->v, subst);
		}
	}
}

static MAC_HOT bool
checker_var_occur(Type_Var *var, Type type)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		return checker_var_occur(var, fun->par) ||
				checker_var_occur(var, fun->ret);
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);

		for (size_t i = 0; i < tuple->len; ++i)
		{
			if (checker_var_occur(var, tuple->items[i]))
			{
				return true;
			}
		}
	}
	else if (tag == TAG_VAR)
	{
		return STR_EQUAL(var->name, ((Type_Var *)TYPE_PTR(type))->name);
	}

	return false;
}

static MAC_HOT void
checker_var_bind(Type_Checker *check, Type_Var *var, Type type, Subst_Tab *subst)
{
	if (TYPE_UNTAG(type) != TAG_VAR ||
		!STR_EQUAL(var->name, ((Type_Var *)TYPE_PTR(type))->name))
	{
		if (!checker_var_occur(var, type))
		{
			subst_tab_set(subst, check->arena, var->name, type);
		}
		else
		{
			Sized_Str msg = format_str(check->arena,
										STR_STATIC("Occurs check failed for var `{S}` in type `{t}`"),
										var, type);
			checker_error(check, msg);
		}
	}
}

static MAC_HOT void
checker_unify(Type_Checker *check, Type type1, Type type2, Subst_Tab *subst)
{
	const Type_Tag tag1 = TYPE_UNTAG(type1);
	const Type_Tag tag2 = TYPE_UNTAG(type2);

	if (tag1 == TAG_FUN && tag2 == TAG_FUN)
	{
		Type_Fun *fun1 = TYPE_PTR(type1);
		Type_Fun *fun2 = TYPE_PTR(type2);
		checker_unify(check, fun1->par, fun2->par, subst);

		Type ret1 = checker_substitute(check, fun1->ret, subst);
		Type ret2 = checker_substitute(check, fun2->ret, subst);
		checker_unify(check, ret1, ret2, subst);
	}
	else if (tag1 == TAG_TUPLE && tag2 == TAG_TUPLE)
	{
		Type_Tuple *tuple1 = TYPE_PTR(type1);
		Type_Tuple *tuple2 = TYPE_PTR(type2);

		if (tuple1->len != tuple2->len)
		{
			Sized_Str msg = format_str(check->arena,
										STR_STATIC("Unification of `{t}` with `{t}` failed"),
										type1, type2);
			checker_error(check, msg);
			return;
		}

		for (size_t i = 0; i < tuple1->len; ++i)
		{
			checker_unify(check, tuple1->items[i], tuple2->items[i], subst);
		}
	}
	else if (tag1 == TAG_VAR)
	{
		checker_var_bind(check, TYPE_PTR(type1), type2, subst);
	}
	else if (tag2 == TAG_VAR)
	{
		checker_var_bind(check, TYPE_PTR(type2), type1, subst);
	}
	else if (tag1 != tag2)
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Unification of `{t}` with `{t}` failed"),
									type1, type2);
		checker_error(check, msg);
	}
}

static MAC_HOT Type
checker_instantiate(Type_Checker *check, const Type_Scheme *scheme)
{
	Subst_Tab subst;
	subst_tab_init(&subst, check->seed);

	for (size_t i = 0; i < scheme->len; ++i)
	{
		subst_tab_set(&subst, check->arena, scheme->vars[i],
						checker_var_new(check));
	}
	return checker_substitute(check, scheme->type, &subst);
}

static MAC_HOT inline Type
checker_infer_if(Type_Checker *check, Scheme_Tab *ctx, Ast_If *node, Subst_Tab *subst)
{
	Subst_Tab tmp;
	subst_tab_init(&tmp, check->seed);
	subst_tab_merge(subst, &tmp, check->arena);

	Type type = checker_var_new(check);
	Type cond = checker_infer_node(check, ctx, node->cond, &tmp);
	Type then = checker_infer_node(check, ctx, node->then, &tmp);
	Type m_else = node->m_else == NULL ? TYPE_UNIT :
					checker_infer_node(check, ctx, node->m_else, &tmp);

	checker_unify(check, checker_substitute(check, then, &tmp), type, &tmp);
	checker_unify(check, checker_substitute(check, m_else, &tmp), type, &tmp);

	checker_unify(check, checker_substitute(check, cond, &tmp), TYPE_BOOL, &tmp);
	return checker_substitute(check, type, &tmp);
}

static MAC_HOT inline Type
checker_infer_let(Type_Checker *check, Scheme_Tab *ctx, Ast_Let *node, Subst_Tab *subst)
{
	return TYPE_NONE;
}

static MAC_HOT inline Type
checker_infer_fun(Type_Checker *check, Scheme_Tab *ctx, Ast_Fun *node, Subst_Tab *subst)
{
	Scheme_Tab tmp;
	scheme_tab_init(&tmp, check->seed);
	scheme_tab_merge(ctx, &tmp, check->arena);

	Type *pars = arena_alloc(check->arena, node->len * sizeof(Type));
	for (size_t i = 0; i < node->len; ++i)
	{
		pars[i] = checker_var_new(check);
		scheme_tab_set(&tmp, check->arena, node->pars[i],
						checker_scheme_new(check, NULL, 0, pars[i]));
	}

	Type ret = checker_infer_node(check, &tmp, node->body, subst);
	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_substitute(check, pars[i - 1], subst);
		ret = checker_fun_new(check, par, ret);
	}

	return ret;
}

static MAC_HOT inline Type
checker_infer_app(Type_Checker *check, Scheme_Tab *ctx, Ast_App *node, Subst_Tab *subst)
{
	if (node->fun->kind == AST_UNARY || node->fun->kind == AST_BINARY ||
		node->fun->kind == AST_TUPLE || node->fun->kind == AST_CONST)
	{
		checker_error(check, STR_STATIC("Type is not a function and can't be applied"));
		return TYPE_NONE;
	}

	Subst_Tab tmp;
	subst_tab_init(&tmp, check->seed);
	subst_tab_merge(subst, &tmp, check->arena);

	Scheme_Tab ctx2;
	scheme_tab_init(&ctx2, check->seed);
	scheme_tab_merge(ctx, &ctx2, check->arena);

	Type ret = checker_var_new(check);
	Type fun = checker_infer_node(check, &ctx2, node->fun, &tmp);

	for (size_t i = node->len; i > 0; --i)
	{
		Type arg = checker_infer_node(check, &ctx2, node->args[i - 1], &tmp);
		ret = checker_fun_new(check, arg, ret);
		checker_substitute_ctx(check, &ctx2, &tmp);
	}

	checker_unify(check, checker_substitute(check, fun, &tmp), ret, &tmp);
	return checker_substitute(check, ret, &tmp);
}

static MAC_HOT inline Type
checker_infer_ident(Type_Checker *check, Scheme_Tab *ctx, Ast_Ident *node, Subst_Tab *subst)
{
	Type_Scheme scheme;
	if (scheme_tab_get(ctx, node->name, &scheme))
	{
		return checker_instantiate(check, &scheme);
	}

	Sized_Str msg = format_str(check->arena,
								STR_STATIC("Unbound identifier `{S}`"),
								node->name);
	checker_error(check, msg);
	return TYPE_NONE;
}

static MAC_HOT inline Type
checker_infer_unary(Type_Checker *check, Scheme_Tab *ctx, Ast_Unary *node, Subst_Tab *subst)
{
	return TYPE_NONE;
}

static MAC_HOT inline Type
checker_infer_binary(Type_Checker *check, Scheme_Tab *ctx, Ast_Binary *node, Subst_Tab *subst)
{
	return TYPE_NONE;
}

static MAC_HOT inline Type
checker_infer_tuple(Type_Checker *check, Scheme_Tab *ctx, Ast_Tuple *node, Subst_Tab *subst)
{
	Type *items = arena_alloc(check->arena, node->len * sizeof(Type));
	for (size_t i = 0; i < node->len; ++i)
	{
		items[i] = checker_infer_node(check, ctx, node->items[i], subst);
	}

	for (size_t i = 0; i < node->len; ++i)
	{
		items[i] = checker_substitute(check, items[i], subst);
	}
	return checker_tuple_new(check, items, node->len);
}

static MAC_HOT Type
checker_infer_node(Type_Checker *check, Scheme_Tab *ctx, Ast_Node *node, Subst_Tab *subst)
{
	switch (node->kind)
	{
		case AST_IF:
			return checker_infer_if(check, ctx, (Ast_If *)node, subst);

		case AST_LET:
			return checker_infer_let(check, ctx, (Ast_Let *)node, subst);

		case AST_FUN:
			return checker_infer_fun(check, ctx, (Ast_Fun *)node, subst);

		case AST_APP:
			return checker_infer_app(check, ctx, (Ast_App *)node, subst);

		case AST_IDENT:
			return checker_infer_ident(check, ctx, (Ast_Ident *)node, subst);

		case AST_UNARY:
			return checker_infer_unary(check, ctx, (Ast_Unary *)node, subst);

		case AST_BINARY:
			return checker_infer_binary(check, ctx, (Ast_Binary *)node, subst);

		case AST_TUPLE:
			return checker_infer_tuple(check, ctx, (Ast_Tuple *)node, subst);

		case AST_CONST:
			return (Type)((Ast_Const *)node)->kind;
	}
	return TYPE_NONE;
}

void
checker_init(Type_Checker *check, Arena *arena, Hash_Default seed, Scheme_Tab *ctx)
{
	check->seed = seed;
	check->tid = 0;
	check->arena = arena;

	check->err = NULL;
	check->succ = true;

	if (ctx == NULL)
	{
		ctx = arena_alloc(arena, sizeof(Scheme_Tab));
		scheme_tab_init(ctx, seed);
	}
	check->ctx = ctx;
}

MAC_HOT bool
checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err)
{
	check->err = err;

	Subst_Tab subst;
	subst_tab_init(&subst, check->seed);

	for (size_t i = 0; i < ast->len; ++i)
	{
		Type type = checker_infer_node(check, check->ctx, ast->items[i], &subst);
		ast->items[i]->type = checker_substitute(check, type, &subst);
	}

	return check->succ;
}
