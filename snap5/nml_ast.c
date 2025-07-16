#include <stddef.h>
#include <stdint.h>

#include "nml_ast.h"
#include "nml_parse.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_type.h"

#define EXPR_NEW(arena, kind, type)	expr_node_new(arena, kind, sizeof(type))
#define PATN_NEW(arena, kind, type)	expr_node_new(arena, kind, sizeof(type))

static MAC_INLINE inline void
const_node_init(Const_Node *node, Const_Kind kind, const void *value)
{
	node->kind = kind;
	switch (kind)
	{
		case CONST_UNIT:
			break;

		case CONST_INT:
			node->value.c_int = *(int32_t *)value;
			break;

		case CONST_FLOAT:
			node->value.c_float = *(double *)value;
			break;

		case CONST_STR:
			node->value.c_str = *(Sized_Str *)value;
			break;

		case CONST_CHAR:
			node->value.c_char = *(char *)value;
			break;

		case CONST_BOOL:
			node->value.c_bool = *(bool *)value;
			break;
	}
}

static MAC_INLINE inline void *
ast_node_new(Arena *arena, Ast_Kind kind, size_t size)
{
	Ast_Node *node =  arena_alloc(arena, size);
	node->kind = kind;
	return (void *)node;
}

static MAC_INLINE inline void *
expr_node_new(Arena *arena, Expr_Kind kind, size_t size)
{
	Expr_Node *node = ast_node_new(arena, AST_EXPR, size);
	node->kind = kind;
	node->type = TYPE_NONE;
	return (void *)node;
}

static MAC_INLINE inline void *
patn_node_new(Arena *arena, Patn_Kind kind, size_t size)
{
	Patn_Node *node = ast_node_new(arena, AST_PATN, size);
	node->kind = kind;
	node->type = TYPE_NONE;
	return (void *)node;
}

Expr_Node *
expr_if_new(Arena *arena, Expr_Node *cond, Expr_Node *then, Expr_Node *m_else)
{
	Expr_If *node = EXPR_NEW(arena, EXPR_IF, Expr_If);
	node->cond = cond;
	node->then = then;
	node->m_else = m_else;
	return (Expr_Node *)node;
}

Expr_Node *
expr_match_new(Arena *arena, Expr_Node *value, Match_Arm *arms, size_t len)
{
	Expr_Match *node = EXPR_NEW(arena, EXPR_MATCH, Expr_Match);
	node->value = value;
	node->arms = arms;
	node->len = len;
	return (Expr_Node *)node;
}

Expr_Node *
expr_let_new(Arena *arena, Sized_Str name, Type hint, Expr_Node *value, Expr_Node *expr)
{
	Expr_Let *node = EXPR_NEW(arena, EXPR_LET, Expr_Let);
	node->name = name;
	node->bind = TYPE_NONE;
	node->hint = hint;
	node->value = value;
	node->expr = expr;
	return (Expr_Node *)node;
}

Expr_Node *
expr_fun_new(Arena *arena, Sized_Str name, Type hint, Sized_Str *pars,
			size_t len, Expr_Node *body, Expr_Node *expr)
{
	Expr_Fun *node = EXPR_NEW(arena, EXPR_FUN, Expr_Fun);
	node->fun = TYPE_NONE;
	node->hint = hint;
	node->rec = TYPE_NONE;
	node->ret = TYPE_NONE;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->expr = expr;
	return (Expr_Node *)node;
}

Expr_Node *
expr_app_new(Arena *arena, Expr_Node *fun, Expr_Node **args, size_t len)
{
	Expr_App *node = EXPR_NEW(arena, EXPR_APP, Expr_App);
	node->fun = fun;
	node->args = args;
	node->len = len;
	return (Expr_Node *)node;
}

Expr_Node *
expr_ident_new(Arena *arena, const Sized_Str name)
{
	Expr_Ident *node = EXPR_NEW(arena, EXPR_IDENT, Expr_Ident);
	node->name = name;
	return (Expr_Node *)node;
}

Expr_Node *
expr_unary_new(Arena *arena, Token_Kind op, Expr_Node *lhs)
{
	Expr_Unary *node = EXPR_NEW(arena, EXPR_UNARY, Expr_Unary);
	node->op = op;
	node->lhs = lhs;
	return (Expr_Node *)node;
}

Expr_Node *
expr_binary_new(Arena *arena, Token_Kind op, Expr_Node *rhs, Expr_Node *lhs)
{
	Expr_Binary *node = EXPR_NEW(arena, EXPR_BINARY, Expr_Binary);
	node->op = op;
	node->rhs = rhs;
	node->lhs = lhs;
	return (Expr_Node *)node;
}

Expr_Node *
expr_tuple_new(Arena *arena, Expr_Node **items, size_t len)
{
	Expr_Tuple *node = EXPR_NEW(arena, EXPR_TUPLE, Expr_Tuple);
	node->items = items;
	node->len = len;
	return (Expr_Node *)node;
}

Expr_Node *
expr_const_new(Arena *arena, Const_Kind kind, const void *value)
{
	Expr_Const *node = EXPR_NEW(arena, EXPR_CONST, Expr_Const);
	const_node_init(&node->cons, kind, value);
	return (Expr_Node *)node;
}
