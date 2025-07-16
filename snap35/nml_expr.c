#include <stddef.h>
#include <stdint.h>

#include "nml_expr.h"
#include "nml_const.h"
#include "nml_parse.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_patn.h"
#include "nml_str.h"
#include "nml_type.h"

static const size_t sizes[] = {
#define NODE(kind, node, _) [EXPR_ ## kind] = sizeof(Expr_ ## node),
#define EXPR
#include "nml_node.h"
#undef EXPR
#undef NODE
};

static MAC_INLINE inline void *
expr_node_new(Arena *arena, Location loc, Expr_Kind kind)
{
	Expr_Node *node = arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->type = TYPE_NONE;
	node->loc = loc;
	return (void *)node;
}

MAC_HOT Expr_Node *
expr_if_new(Arena *arena, Location loc, Expr_Node *cond,
			Expr_Node *b_then, Expr_Node *b_else)
{
	Expr_If *node = expr_node_new(arena, loc, EXPR_IF);
	node->cond = cond;
	node->b_then = b_then;
	node->b_else = b_else;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_match_new(Arena *arena, Location loc, Expr_Node *value,
				struct Expr_Match_Arm *arms, size_t len)
{
	Expr_Match *node = expr_node_new(arena, loc, EXPR_MATCH);
	node->value = value;
	node->arms = arms;
	node->len = len;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_let_new(Arena *arena, Location loc, Type hint, Sized_Str name,
			Expr_Node *value, Expr_Node *expr)
{
	Expr_Let *node = expr_node_new(arena, loc, EXPR_LET);
	node->name = name;
	node->hint = hint;
	node->value = value;
	node->expr = expr;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_fun_new(Arena *arena, Location loc, Type hint, Sized_Str name,
			Sized_Str *pars, size_t len, Expr_Node *body, Expr_Node *expr)
{
	Expr_Fun *node = expr_node_new(arena, loc, EXPR_FUN);
	node->fun = TYPE_NONE;
	node->rec = TYPE_NONE;
	node->hint = hint;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->expr = expr;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_lambda_new(Arena *arena, Location loc, Sized_Str *pars, size_t len, Expr_Node *body)
{
	Expr_Lambda *node = expr_node_new(arena, loc, EXPR_LAMBDA);
	node->pars = pars;
	node->len = len;
	node->body = body;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_app_new(Arena *arena, Location loc, Expr_Node *fun, Expr_Node **args, size_t len)
{
	Expr_App *node = expr_node_new(arena, loc, EXPR_APP);
	node->fun = fun;
	node->args = args;
	node->len = len;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_ident_new(Arena *arena, Location loc, Sized_Str name)
{
	Expr_Ident *node = expr_node_new(arena, loc, EXPR_IDENT);
	node->name = name;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_binary_new(Arena *arena, Location loc, Token_Kind op,
				Expr_Node *rhs, Expr_Node *lhs)
{
	Expr_Binary *node = expr_node_new(arena, loc, EXPR_BINARY);
	node->op = op;
	node->rhs = rhs;
	node->lhs = lhs;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_tuple_new(Arena *arena, Location loc, Expr_Node **items, size_t len)
{
	Expr_Tuple *node = expr_node_new(arena, loc, EXPR_TUPLE);
	node->items = items;
	node->len = len;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_const_new(Arena *arena, Location loc, Const_Value value)
{
	Expr_Const *node = expr_node_new(arena, loc, EXPR_CONST);
	node->value = value;
	return (Expr_Node *)node;
}
