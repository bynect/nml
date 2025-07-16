#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "nml_expr.h"
#include "nml_lit.h"
#include "nml_parse.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_patn.h"
#include "nml_str.h"
#include "nml_type.h"

#define EXPR_NEW(_node, _kind, _loc)					\
	arena_alloc(arena, sizeof(_node));					\
	MAC_CAST(Expr_Node *, node)->kind = _kind;			\
	MAC_CAST(Expr_Node *, node)->type = TYPE_NONE;		\
	MAC_CAST(Expr_Node *, node)->loc = _loc

#define EXPR_COPY(_node)						\
	arena_alloc(arena, sizeof(_node));			\
	memcpy(copy, node, sizeof(Expr_Node))

MAC_HOT Expr_Node *
expr_if_new(Arena *arena, Location loc, Expr_Node *cond,
			Expr_Node *b_then, Expr_Node *b_else)
{
	Expr_If *node = EXPR_NEW(Expr_If, EXPR_IF, loc);
	node->cond = cond;
	node->b_then = b_then;
	node->b_else = b_else;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_match_new(Arena *arena, Location loc, Expr_Node *value,
				struct Expr_Match_Arm *arms, size_t len)
{
	Expr_Match *node = EXPR_NEW(Expr_Match, EXPR_MATCH, loc);
	node->value = value;
	node->arms = arms;
	node->len = len;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_let_new(Arena *arena, Location loc, Type hint, Sized_Str name,
			Expr_Node *value, Expr_Node *expr)
{
	Expr_Let *node = EXPR_NEW(Expr_Let, EXPR_LET, loc);
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
	Expr_Fun *node = EXPR_NEW(Expr_Fun, EXPR_FUN, loc);
	node->fun = TYPE_NONE;
	node->hint = hint;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->expr = expr;
	node->label = 0;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_lambda_new(Arena *arena, Location loc, Sized_Str *pars, size_t len, Expr_Node *body)
{
	Expr_Lambda *node = EXPR_NEW(Expr_Lambda, EXPR_LAMBDA, loc);
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->label = 0;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_app_new(Arena *arena, Location loc, Expr_Node *fun, Expr_Node **args, size_t len)
{
	Expr_App *node = EXPR_NEW(Expr_App, EXPR_APP, loc);
	node->fun = fun;
	node->args = args;
	node->len = len;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_ident_new(Arena *arena, Location loc, Sized_Str name)
{
	Expr_Ident *node = EXPR_NEW(Expr_Ident, EXPR_IDENT, loc);
	node->name = name;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_cons_new(Arena *arena, Location loc, Sized_Str name, Expr_Node *value)
{
	Expr_Cons *node = EXPR_NEW(Expr_Cons, EXPR_CONS, loc);
	node->name = name;
	node->value = value;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_tuple_new(Arena *arena, Location loc, Expr_Node **items, size_t len)
{
	Expr_Tuple *node = EXPR_NEW(Expr_Tuple, EXPR_TUPLE, loc);
	node->items = items;
	node->len = len;
	return (Expr_Node *)node;
}

MAC_HOT Expr_Node *
expr_lit_new(Arena *arena, Location loc, Lit_Value value)
{
	Expr_Lit *node = EXPR_NEW(Expr_Lit, EXPR_LIT, loc);
	node->value = value;
	return (Expr_Node *)node;
}

//MAC_HOT Expr_Node *
//expr_node_copy(Arena *arena, Expr_Node *node)
//{
//	switch (node->kind)
//	{
//		case EXPR_IF:
//			return (Expr_Node *)expr_if_copy(arena, node);
//
//		case EXPR_MATCH:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_LET:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_FUN:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_LAMBDA:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_APP:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_IDENT:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_TUPLE:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//
//		case EXPR_LIT:
//			return (Expr_Node *)expr_lit_copy(arena, node);
//	}
//}
//
//MAC_HOT Expr_If *
//expr_if_copy(Arena *arena, Expr_If *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_Match *
//expr_match_copy(Arena *arena, Expr_Match *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_Let *
//expr_let_copy(Arena *arena, Expr_Let *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_Fun *
//expr_fun_copy(Arena *arena, Expr_Fun *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_Lambda *
//expr_lambda_copy(Arena *arena, Expr_Lambda *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_App *
//expr_app_copy(Arena *arena, Expr_App *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_Ident *
//expr_ident_copy(Arena *arena, Expr_Ident *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
//
//MAC_HOT Expr_Tuple *
//expr_tuple_copy(Arena *arena, Expr_Tuple *node)
//{
//	Expr_Tuple *copy = EXPR_COPY(Expr_Tuple);
//	copy->items = arena_alloc(arena, node->len);
//	copy->len = node->len;
//
//	for (size_t i = 0; i < copy->len; ++i)
//	{
//		copy->items[i] = expr_node_copy(arena, node->items[i]);
//	}
//	return copy;
//}
//
//MAC_HOT Expr_Lit *
//expr_lit_copy(Arena *arena, Expr_Lit *node)
//{
//	Expr_Lit *copy = EXPR_COPY(Expr_Lit);
//	copy->value = node->value;
//	return copy;
//}
