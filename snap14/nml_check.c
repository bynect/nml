#include "nml_const.h"
#include "nml_dbg.h"
#include "nml_dump.h"
#include "nml_patn.h"
#include "nml_sig.h"
#include "nml_type.h"
#include "nml_expr.h"
#include "nml_err.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_fmt.h"
#include "nml_check.h"

static MAC_HOT void checker_annot_ast(Type_Checker *check, Expr_Node *node, Type_Tab *ctx);
static MAC_HOT void checker_constr_ast(Type_Checker *check, Expr_Node *node, Constr_Buf *constr);
static MAC_HOT void checker_unify(Type_Checker *check, Subst_Tab *subst, Type_Constr constr);

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
	return TYPE_TAG(TAG_VAR, check->tid++);
}

static MAC_INLINE inline Type
checker_alias(Type_Checker *check, Location loc, bool *subst, Type type)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		bool tmp = false;

		Type par = checker_alias(check, loc, &tmp, fun->par);
		Type ret = checker_alias(check, loc, &tmp, fun->ret);

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
			items[i] = checker_alias(check, loc, &tmp, tuple->items[i]);
		}

		if (tmp)
		{
			arena_unlock(check->arena, items);
			return checker_tuple_new(check, items, tuple->len);
		}
		arena_reset(check->arena, items);
	}
	else if (tag == TAG_IDENT)
	{
		Type_Ident *ident = TYPE_PTR(type);
		if (MAC_LIKELY(type_tab_get(&check->env, ident->name, &type)))
		{
			*subst = true;
		}
		else
		{
			Sized_Str msg = format_str(check->arena, STR_STATIC("Unbound type `{S}`"),
										ident->name);
			checker_error(check, (Location) {0, 0}, msg);
		}
	}
	return type;
}

static MAC_HOT void
checker_annot_patn(Type_Checker *check, Patn_Node *node, Type_Tab *ctx)
{
	node->type = checker_var_new(check);
	if (node->kind == PATN_IDENT)
	{
		Patn_Ident *ident = (Patn_Ident *)node;
		if (!STR_EQUAL(ident->name, STR_STATIC("_")))
		{
			type_tab_insert(ctx, check->arena, ident->name, node->type);
		}
	}
	else if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			checker_annot_patn(check, tuple->items[i], ctx);
		}
	}
}

static MAC_HOT inline void
checker_annot_expr_if(Type_Checker *check, Expr_If *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	checker_annot_ast(check, node->cond, ctx);
	checker_annot_ast(check, node->b_then, ctx);
	checker_annot_ast(check, node->b_else, ctx);
}

static MAC_HOT inline void
checker_annot_expr_match(Type_Checker *check, Expr_Match *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	checker_annot_ast(check, node->value, ctx);

	Str_Set set;
	str_set_init(&set, check->seed);

	Type_Tab tmp;
	type_tab_init(&tmp, check->seed);

	for (size_t i = 0; i < node->len; ++i)
	{
		// TODO: Improve tab and set performance
		// Try to not reset and copy at every iteration
		for (size_t i = 0; i < node->len; ++i)
		{
			pattern_check(node->arms[i].patn, check->arena, &set, check->err);
			str_set_reset(&set);
		}

		type_tab_union(ctx, &tmp, check->arena);
		checker_annot_patn(check, node->arms[i].patn, &tmp);

		checker_annot_ast(check, node->arms[i].expr, &tmp);
		type_tab_reset(&tmp);
	}
}

static MAC_HOT inline void
checker_annot_expr_let(Type_Checker *check, Expr_Let *node, Type_Tab *ctx)
{
	node->bind = checker_var_new(check);
	node->node.type = checker_var_new(check);
	checker_annot_ast(check, node->value, ctx);

	Type type;
	bool shad = type_tab_get(ctx, node->name, &type);
	type_tab_insert(ctx, check->arena, node->name, node->bind);

	if (node->expr != NULL)
	{
		checker_annot_ast(check, node->expr, ctx);

		if (shad)
		{
			type_tab_insert(ctx, check->arena, node->name, type);
		}
		else
		{
			type_tab_remove(ctx, node->name);
		}
	}
}

static MAC_HOT inline void
checker_annot_expr_fun(Type_Checker *check, Expr_Fun *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	node->rec = checker_var_new(check);

	bool anon = STR_EQUAL(node->name, STR_STATIC("@anon"));
	bool shad = false;

	Type type;
	if (!anon)
	{
		shad = type_tab_get(ctx, node->name, &type);
		type_tab_insert(ctx, check->arena, node->name, node->rec);
	}

	Type_Tab scope;
	type_tab_init(&scope, check->seed);
	type_tab_union(ctx, &scope, check->arena);

	node->ret = checker_var_new(check);
	node->fun = node->ret;

	for (size_t i = node->len; i > 0; --i)
	{
		Type par = checker_var_new(check);
		node->fun = checker_fun_new(check, par, node->fun);
		type_tab_insert(&scope, check->arena, node->pars[i - 1], par);
	}

	checker_annot_ast(check, node->body, &scope);
	if (node->expr != NULL)
	{
		checker_annot_ast(check, node->expr, ctx);

		if (shad)
		{
			type_tab_insert(ctx, check->arena, node->name, type);
		}
		else if (!anon)
		{
			type_tab_remove(ctx, node->name);
		}
	}
}

static MAC_HOT inline void
checker_annot_expr_app(Type_Checker *check, Expr_App *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	checker_annot_ast(check, node->fun, ctx);

	if (node->fun->kind == EXPR_UNARY || node->fun->kind == EXPR_BINARY ||
		node->fun->kind == EXPR_TUPLE || node->fun->kind == EXPR_CONST)
	{
		checker_error(check, node->fun->loc, STR_STATIC("Only functions can be applied"));
		return;
	}


	for (size_t i = 0; i < node->len; ++i)
	{
		checker_annot_ast(check, node->args[i], ctx);
	}
}

static MAC_HOT inline void
checker_annot_expr_ident(Type_Checker *check, Expr_Ident *node, Type_Tab *ctx)
{
	Type type = TYPE_NONE;
	if (!type_tab_get(ctx, node->name, &type))
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Unbound identifier `{S}`"),
									node->name);
		checker_error(check, node->node.loc, msg);
	}
	node->node.type = type;
}

static MAC_HOT inline void
checker_annot_expr_unary(Type_Checker *check, Expr_Unary *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	checker_annot_ast(check, node->lhs, ctx);
}

static MAC_HOT inline void
checker_annot_expr_binary(Type_Checker *check, Expr_Binary *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	checker_annot_ast(check, node->rhs, ctx);
	checker_annot_ast(check, node->lhs, ctx);
}

static MAC_HOT inline void
checker_annot_expr_tuple(Type_Checker *check, Expr_Tuple *node, Type_Tab *ctx)
{
	node->node.type = checker_var_new(check);
	for (size_t i = 0; i < node->len; ++i)
	{
		checker_annot_ast(check, node->items[i], ctx);
	}
}

static MAC_HOT void
checker_annot_ast(Type_Checker *check, Expr_Node *node, Type_Tab *ctx)
{
	switch (node->kind)
	{
		case EXPR_IF:
			checker_annot_expr_if(check, (Expr_If *)node, ctx);
			break;

		case EXPR_MATCH:
			checker_annot_expr_match(check, (Expr_Match *)node, ctx);
			break;

		case EXPR_LET:
			checker_annot_expr_let(check, (Expr_Let *)node, ctx);
			break;

		case EXPR_FUN:
			checker_annot_expr_fun(check, (Expr_Fun *)node, ctx);
			break;

		case EXPR_APP:
			checker_annot_expr_app(check, (Expr_App *)node, ctx);
			break;

		case EXPR_IDENT:
			checker_annot_expr_ident(check, (Expr_Ident *)node, ctx);
			break;

		case EXPR_UNARY:
			checker_annot_expr_unary(check, (Expr_Unary *)node, ctx);
			break;

		case EXPR_BINARY:
			checker_annot_expr_binary(check, (Expr_Binary *)node, ctx);
			break;

		case EXPR_TUPLE:
			checker_annot_expr_tuple(check, (Expr_Tuple *)node, ctx);
			break;

		case EXPR_CONST:
			node->type = checker_var_new(check);
			break;
	}
}

static MAC_INLINE inline void
checker_constr_set(Constr_Buf *constr, Type t1, Type t2)
{
	constr_buf_push(constr, TYPE_CONSTR(t1, t2));
}


static MAC_HOT void
checker_constr_const(Const_Kind kind, Type type, Constr_Buf *constr)
{
	const Type consts[] = {
#define TAG(name, _, kind)	[CONST_ ## name] = TYPE_ ## name,
#define TYPE
#define BASE
#include "nml_tag.h"
#undef BASE
#undef TYPE
#undef TAG
 	};
	checker_constr_set(constr, type, consts[kind]);
}

static MAC_HOT void
checker_constr_patn(Patn_Node *node, Constr_Buf *constr)
{
	if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			checker_constr_patn(tuple->items[i], constr);
		}
	}
	else if (node->kind == PATN_CONST)
	{
		checker_constr_const(((Patn_Const *)node)->value.kind, node->type, constr);
	}
}

static MAC_HOT inline void
checker_constr_expr_if(Type_Checker *check, Expr_If *node, Constr_Buf *constr)
{
	checker_constr_ast(check, node->cond, constr);
	checker_constr_set(constr, node->cond->type, TYPE_BOOL);

	checker_constr_ast(check, node->b_then, constr);
	checker_constr_set(constr, node->b_then->type, node->node.type);

	checker_constr_ast(check, node->b_else, constr);
	checker_constr_set(constr, node->node.type, node->b_else->type);
}

static MAC_HOT inline void
checker_constr_expr_match(Type_Checker *check, Expr_Match *node, Constr_Buf *constr)
{
	checker_constr_ast(check, node->value, constr);

	for (size_t i = 0; i < node->len; ++i)
	{
		checker_constr_patn(node->arms[i].patn, constr);
		checker_constr_ast(check, node->arms[i].expr, constr);

		checker_constr_set(constr, node->arms[i].patn->type, node->value->type);
		checker_constr_set(constr, node->arms[i].expr->type, node->node.type);
	}
}

static MAC_HOT inline void
checker_constr_expr_let(Type_Checker *check, Expr_Let *node, Constr_Buf *constr)
{
	checker_constr_ast(check, node->value, constr);
	checker_constr_set(constr, node->bind, node->value->type);

	if (node->hint != TYPE_NONE)
	{
		// TODO: Add detailed error message for erroneous hints
		bool tmp = false;
		Type hint = checker_alias(check, node->node.loc, &tmp, node->hint);
		checker_constr_set(constr, hint, node->value->type);
	}

	if (node->expr == NULL)
	{
		checker_constr_set(constr, node->node.type, node->value->type);
	}
	else
	{
		checker_constr_ast(check, node->expr, constr);
		checker_constr_set(constr, node->node.type, node->expr->type);
	}
}

static MAC_HOT inline void
checker_constr_expr_fun(Type_Checker *check, Expr_Fun *node, Constr_Buf *constr)
{
	checker_constr_ast(check, node->body, constr);
	checker_constr_set(constr, node->fun, node->rec);
	checker_constr_set(constr, node->body->type, node->ret);

	if (node->hint != TYPE_NONE)
	{
		// TODO: Add detailed error message for erroneous hints
		bool tmp = false;
		Type hint = checker_alias(check, node->node.loc, &tmp, node->hint);
		checker_constr_set(constr, hint, node->body->type);
	}

	if (node->expr == NULL)
	{
		checker_constr_set(constr, node->node.type, node->fun);
	}
	else
	{
		checker_constr_ast(check, node->expr, constr);
		checker_constr_set(constr, node->node.type, node->expr->type);
	}
}

static MAC_HOT inline void
checker_constr_expr_app(Type_Checker *check, Expr_App *node, Constr_Buf *constr)
{
	checker_constr_ast(check, node->fun, constr);

	Type ret = node->node.type;
	for (size_t i = node->len; i > 0; --i)
	{
		checker_constr_ast(check, node->args[i - 1], constr);
		ret = checker_fun_new(check, node->args[i - 1]->type, ret);
	}

	checker_constr_set(constr, node->fun->type, ret);
}

static MAC_HOT inline void
checker_constr_expr_unary(Type_Checker *check, Expr_Unary *node, Constr_Buf *constr)
{
	Sized_Str op;
	if (node->op == TOK_MINUS)
	{
		if (MAC_UNLIKELY(node->lhs->kind == EXPR_CONST &&
			((Expr_Const *)node->lhs)->value.kind == CONST_FLOAT))
		{
			op = STR_STATIC("~-.");
		}
		else
		{
			op = STR_STATIC("~-");
		}
	}
	else if (node->op == TOK_MINUSF)
	{
		op = STR_STATIC("~-.");
	}
	else if (node->op == TOK_BANG)
	{
		op = STR_STATIC("~!");
	}
	else
	{
		Sized_Str msg = format_str(check->arena,
								STR_STATIC("Invalid unary operator `{K}`"), node->op);
		checker_error(check, node->node.loc, msg);
		return;
	}

	Type type;
	if (!type_tab_get(check->ctx, op, &type))
	{
		Sized_Str msg = format_str(check->arena,
								STR_STATIC("Invalid unary operator `{K}`"), node->op);
		checker_error(check, node->node.loc, msg);
		return;
	}

	checker_constr_ast(check, node->lhs, constr);

	Type_Fun *fun = TYPE_PTR(type);
	checker_constr_set(constr, node->lhs->type, fun->par);
	checker_constr_set(constr, node->node.type, fun->ret);
}

static MAC_HOT inline void
checker_constr_expr_binary(Type_Checker *check, Expr_Binary *node, Constr_Buf *constr)
{
	bool err;
	Type type;
	switch (node->op)
	{
#define TOK(kind, str, _, __)										\
	case TOK_ ## kind:												\
		err = !type_tab_get(check->ctx, STR_STATIC(str), &type);	\
		break;
#define OPER
#include "nml_tok.h"
#undef OPER
#undef TOK

		default:
			err = true;
			break;
	}

	if (err)
	{
		Sized_Str msg = format_str(check->arena,
								STR_STATIC("Invalid binary operator `{K}`"), node->op);
		checker_error(check, node->node.loc, msg);
		return;
	}

	checker_constr_ast(check, node->rhs, constr);
	checker_constr_ast(check, node->lhs, constr);

	Type_Fun *fun = TYPE_PTR(type);
	Type rhs = fun->par;

	fun = TYPE_PTR(fun->ret);
	Type lhs = fun->par;

	checker_constr_set(constr, node->rhs->type, rhs);
	checker_constr_set(constr, node->lhs->type, lhs);
	checker_constr_set(constr, node->node.type, fun->ret);
}

static MAC_HOT inline void
checker_constr_expr_tuple(Type_Checker *check, Expr_Tuple *node, Constr_Buf *constr)
{
	Type *items = arena_alloc(check->arena, node->len * sizeof(Type));
	for (size_t i = 0; i < node->len; ++i)
	{
		checker_constr_ast(check, node->items[i], constr);
		items[i] = node->items[i]->type;
	}

	checker_constr_set(constr, node->node.type,
						checker_tuple_new(check, items, node->len));
}

static MAC_HOT void
checker_constr_ast(Type_Checker *check, Expr_Node *node, Constr_Buf *constr)
{
	switch (node->kind)
	{
		case EXPR_IF:
			checker_constr_expr_if(check, (Expr_If *)node, constr);
			break;

		case EXPR_MATCH:
			checker_constr_expr_match(check, (Expr_Match *)node, constr);
			break;

		case EXPR_LET:
			checker_constr_expr_let(check, (Expr_Let *)node, constr);
			break;

		case EXPR_FUN:
			checker_constr_expr_fun(check, (Expr_Fun *)node, constr);
			break;

		case EXPR_APP:
			checker_constr_expr_app(check, (Expr_App *)node, constr);
			break;

		case EXPR_IDENT:
			break;

		case EXPR_UNARY:
			checker_constr_expr_unary(check, (Expr_Unary *)node, constr);
			break;

		case EXPR_BINARY:
			checker_constr_expr_binary(check, (Expr_Binary *)node, constr);
			break;

		case EXPR_TUPLE:
			checker_constr_expr_tuple(check, (Expr_Tuple *)node, constr);
			break;

		case EXPR_CONST:
			checker_constr_const(((Expr_Const *)node)->value.kind, node->type, constr);
			break;
	}
}

static MAC_HOT Type
checker_subst_type(Type_Checker *check, Subst_Tab *subst, Type type)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		Type par = checker_subst_type(check, subst, fun->par);
		Type ret = checker_subst_type(check, subst, fun->ret);
		return checker_fun_new(check, par, ret);
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		Type *items = arena_alloc(check->arena, tuple->len * sizeof(Type));

		for (size_t i = 0; i < tuple->len; ++i)
		{
			items[i] = checker_subst_type(check, subst, tuple->items[i]);
		}

		return checker_tuple_new(check, items, tuple->len);
	}
	else if (tag == TAG_VAR)
	{
		if (subst_tab_get(subst, TYPE_TID(type), &type))
		{
			return checker_subst_type(check, subst, type);
		}
	}
	return type;
}

static MAC_HOT void
checker_subst_constr(Type_Checker *check, Subst_Tab *subst, Constr_Buf *constr)
{
	for (size_t i = 0; i < constr->len; ++i)
	{
		constr->buf[i].t1 = checker_subst_type(check, subst, constr->buf[i].t1);
		constr->buf[i].t2 = checker_subst_type(check, subst, constr->buf[i].t2);
	}
}

static MAC_HOT void
checker_subst_patn(Type_Checker *check, Subst_Tab *subst, Patn_Node *node)
{
	node->type = checker_subst_type(check, subst, node->type);
	if (node->kind == PATN_TUPLE)
	{
		Patn_Tuple *tuple = (Patn_Tuple *)node;
		for (size_t i = 0; i < tuple->len; ++i)
		{
			checker_subst_patn(check, subst, tuple->items[i]);
		}
	}
}

static MAC_HOT void
checker_subst_ast(Type_Checker *check, Subst_Tab *subst, Expr_Node *node)
{
	node->type = checker_subst_type(check, subst, node->type);
	switch (node->kind)
	{
		case EXPR_IF:
			checker_subst_ast(check, subst, ((Expr_If *)node)->cond);
			checker_subst_ast(check, subst, ((Expr_If *)node)->b_then);
			checker_subst_ast(check, subst, ((Expr_If *)node)->b_else);
			break;

		case EXPR_MATCH:
			checker_subst_ast(check, subst, ((Expr_Match *)node)->value);

			for (size_t i = 0; i < ((Expr_Match *)node)->len; ++i)
			{
				checker_subst_patn(check, subst, ((Expr_Match *)node)->arms[i].patn);
				checker_subst_ast(check, subst, ((Expr_Match *)node)->arms[i].expr);
			}
			break;

		case EXPR_LET:
			checker_subst_ast(check, subst, ((Expr_Let *)node)->value);
			if (((Expr_Let *)node)->expr != NULL)
			{
				checker_subst_ast(check, subst, ((Expr_Let *)node)->expr);
			}
			break;

		case EXPR_FUN:
			checker_subst_ast(check, subst, ((Expr_Fun *)node)->body);
			if (((Expr_Fun *)node)->expr != NULL)
			{
				checker_subst_ast(check, subst, ((Expr_Fun *)node)->expr);
			}
			break;

		case EXPR_APP:
			checker_subst_ast(check, subst, ((Expr_App *)node)->fun);
			for (size_t i = 0; i < ((Expr_App *)node)->len; ++i)
			{
				checker_subst_ast(check, subst, ((Expr_App *)node)->args[i]);
			}
			break;

		case EXPR_IDENT:
			break;

		case EXPR_UNARY:
			checker_subst_ast(check, subst, ((Expr_Unary *)node)->lhs);
			break;

		case EXPR_BINARY:
			checker_subst_ast(check, subst, ((Expr_Binary *)node)->rhs);
			checker_subst_ast(check, subst, ((Expr_Binary *)node)->lhs);
			break;

		case EXPR_TUPLE:
			for (size_t i = 0; i < ((Expr_Tuple *)node)->len; ++i)
			{
				checker_subst_ast(check, subst, ((Expr_Tuple *)node)->items[i]);
			}
			break;

		case EXPR_CONST:
			break;
	}
}

static MAC_HOT void
checker_compose(Type_Checker *check, const Subst_Tab *src, Subst_Tab *dest)
{
	for (size_t i = 0; i < src->size; ++i)
	{
		Subst_Tab_Buck *buck = src->bucks + i;
		if (buck->w_key)
		{
			buck->v = checker_subst_type(check, dest, buck->v);
		}
	}
	subst_tab_union(src, dest, check->arena);
}

static MAC_HOT bool
checker_occur(Type_Id var, Type type)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		Type_Fun *fun = TYPE_PTR(type);
		return checker_occur(var, fun->par) ||
				checker_occur(var, fun->ret);
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);

		for (size_t i = 0; i < tuple->len; ++i)
		{
			if (checker_occur(var, tuple->items[i]))
			{
				return true;
			}
		}
	}
	else if (tag == TAG_VAR)
	{
		return var == TYPE_TID(type);
	}

	return false;
}

static MAC_HOT void
checker_unify_var(Type_Checker *check, Subst_Tab *subst, Type_Id var, Type type)
{
	if (TYPE_UNTAG(type) == TAG_VAR)
	{
		if (TYPE_TID(type) != var)
		{
			subst_tab_insert(subst, check->arena, var, type);
		}
		return;
	}

	if (!checker_occur(var, type))
	{
		// TODO: Implement polymorphic type var
		subst_tab_insert(subst, check->arena, var, type);
	}
	else
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Occurs check failed for var {i} in type `{t}`"),
									var, type);
		checker_error(check, (Location) {0, 0}, msg);
	}
}

static MAC_HOT void
checker_unify_constr(Type_Checker *check, Subst_Tab *subst, Constr_Buf *constr)
{
	if (!constr_buf_pop(constr))
	{
		return;
	}

	Subst_Tab tmp;
	subst_tab_init(&tmp, check->seed);

	checker_unify(check, &tmp, constr->buf[constr->len]);
	checker_subst_constr(check, &tmp, constr);

	checker_unify_constr(check, &tmp, constr);
	checker_compose(check, &tmp, subst);
	++constr->len;
}

static MAC_HOT void
checker_unify(Type_Checker *check, Subst_Tab *subst, Type_Constr constr)
{
	const Type_Tag tag1 = TYPE_UNTAG(constr.t1);
	const Type_Tag tag2 = TYPE_UNTAG(constr.t2);

	if (tag1 == TAG_FUN && tag2 == TAG_FUN)
	{
		Type_Fun *fun1 = TYPE_PTR(constr.t1);
		Type_Fun *fun2 = TYPE_PTR(constr.t2);

		Constr_Buf buf;
		constr_buf_init(&buf, check->arena);

		constr_buf_push(&buf, TYPE_CONSTR(fun1->par, fun2->par));
		constr_buf_push(&buf, TYPE_CONSTR(fun1->ret, fun2->ret));

		checker_unify_constr(check, subst, &buf);
		constr_buf_free(&buf);
	}
	else if (tag1 == TAG_TUPLE && tag2 == TAG_TUPLE)
	{
		Type_Tuple *tuple1 = TYPE_PTR(constr.t1);
		Type_Tuple *tuple2 = TYPE_PTR(constr.t2);

		if (tuple1->len != tuple2->len)
		{
			Sized_Str msg = format_str(check->arena,
										STR_STATIC("Unification of `{t}` with `{t}` failed"),
										constr.t1, constr.t2);
			checker_error(check, (Location) {0, 0}, msg);
			return;
		}

		Constr_Buf buf;
		constr_buf_init(&buf, check->arena);

		for (size_t i = 0; i < tuple1->len; ++i)
		{
			constr_buf_push(&buf, TYPE_CONSTR(tuple1->items[i], tuple2->items[i]));
		}

		checker_unify_constr(check, subst, &buf);
		constr_buf_free(&buf);
	}
	else if (tag1 == TAG_VAR)
	{
		checker_unify_var(check, subst, TYPE_TID(constr.t1), constr.t2);
	}
	else if (tag2 == TAG_VAR)
	{
		checker_unify_var(check, subst, TYPE_TID(constr.t2), constr.t1);
	}
	else if (tag1 != tag2)
	{
		Sized_Str msg = format_str(check->arena,
									STR_STATIC("Unification of `{t}` with `{t}` failed"),
									constr.t1, constr.t2);
		checker_error(check, (Location) {0, 0}, msg);
	}
}

void
checker_init(Type_Checker *check, Arena *arena, Hash_Default seed, Type_Tab *ctx)
{
	check->seed = seed;
	check->tid = 0;
	check->arena = arena;

	check->err = NULL;
	check->succ = true;

	if (ctx == NULL)
	{
		ctx = arena_alloc(arena, sizeof(Type_Tab));
		type_tab_init(ctx, seed);
	}
	check->ctx = ctx;

	type_tab_init(&check->env, seed);
}

MAC_HOT void
checker_sig(Type_Checker *check, Sig_Node *node)
{
	if (node->kind == SIG_TYPE)
	{
		Sig_Type *sig = (Sig_Type *)node;
		bool tmp = false;

		Type type = checker_alias(check, node->loc, &tmp, sig->type);
		type_tab_insert(&check->env, check->arena, sig->name, type);
	}
}

MAC_HOT bool
checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err)
{
	check->err = err;
	bool succ = true;

	Constr_Buf constr;
	constr_buf_init(&constr, check->arena);

	Subst_Tab subst;
	subst_tab_init(&subst, check->seed);

	for (size_t i = 0; i < ast->len; ++i)
	{
		if (MAC_UNLIKELY(ast->items[i].sig))
		{
			checker_sig(check, (Sig_Node *)ast->items[i].node);
			continue;
		}
		Expr_Node *node = ast->items[i].node;

		checker_annot_ast(check, node, check->ctx);
		if (MAC_UNLIKELY(!check->succ))
		{
			check->succ = true;
			succ = false;
			continue;
		}

		checker_constr_ast(check, node, &constr);
		if (MAC_UNLIKELY(!check->succ))
		{
			check->succ = true;
			succ = false;
			continue;
		}

		checker_unify_constr(check, &subst, &constr);
		if (MAC_UNLIKELY(!check->succ))
		{
			check->succ = true;
			succ = false;
			continue;
		}

		checker_subst_ast(check, &subst, node);
		if (node->kind == EXPR_FUN)
		{
			Expr_Fun *fun = (Expr_Fun *)node;
			if (fun->expr == NULL && !STR_EQUAL(fun->name, STR_STATIC("@anon")))
			{
				type_tab_insert(check->ctx, check->arena, fun->name, fun->fun);
			}
		}
		else if (node->kind == EXPR_LET)
		{
			Expr_Let *let = (Expr_Let *)node;
			if (let->expr == NULL)
			{
				type_tab_insert(check->ctx, check->arena, let->name, node->type);
			}
		}
	}

	constr_buf_free(&constr);
	return succ;
}
