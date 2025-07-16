#include <stddef.h>
#include <stdint.h>

#include "nml_ast.h"
#include "nml_const.h"
#include "nml_parse.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_patn.h"
#include "nml_str.h"
#include "nml_type.h"

static const size_t sizes[] = {
#define NODE(kind, node, _) [AST_ ## kind] = sizeof(Ast_ ## node),
#define EXPR
#include "nml_node.h"
#undef EXPR
#undef NODE
};

static MAC_INLINE inline void *
ast_node_new(Arena *arena, Location loc, Ast_Kind kind)
{
	Ast_Node *node = arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->type = TYPE_NONE;
	node->loc = loc;
	return (void *)node;
}

MAC_HOT Ast_Node *
ast_if_new(Arena *arena, Location loc, Ast_Node *cond, Ast_Node *b_then, Ast_Node *b_else)
{
	Ast_If *node = ast_node_new(arena, loc, AST_IF);
	node->cond = cond;
	node->b_then = b_then;
	node->b_else = b_else;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_match_new(Arena *arena, Location loc, Ast_Node *value, struct Ast_Match_Arm *arms, size_t len)
{
	Ast_Match *node = ast_node_new(arena, loc, AST_MATCH);
	node->value = value;
	node->arms = arms;
	node->len = len;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_let_new(Arena *arena, Location loc, Sized_Str name, Type hint, Ast_Node *value, Ast_Node *expr)
{
	Ast_Let *node = ast_node_new(arena, loc, AST_LET);
	node->name = name;
	node->bind = TYPE_NONE;
	node->hint = hint;
	node->value = value;
	node->expr = expr;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_fun_new(Arena *arena, Location loc, Sized_Str name, Type hint, Sized_Str *pars,
			size_t len, Ast_Node *body, Ast_Node *expr)
{
	Ast_Fun *node = ast_node_new(arena, loc, AST_FUN);
	node->fun = TYPE_NONE;
	node->hint = hint;
	node->rec = TYPE_NONE;
	node->ret = TYPE_NONE;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->expr = expr;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_app_new(Arena *arena, Location loc, Ast_Node *fun, Ast_Node **args, size_t len)
{
	Ast_App *node = ast_node_new(arena, loc, AST_APP);
	node->fun = fun;
	node->args = args;
	node->len = len;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_ident_new(Arena *arena, Location loc, Sized_Str name)
{
	Ast_Ident *node = ast_node_new(arena, loc, AST_IDENT);
	node->name = name;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_unary_new(Arena *arena, Location loc, Token_Kind op, Ast_Node *lhs)
{
	Ast_Unary *node = ast_node_new(arena, loc, AST_UNARY);
	node->op = op;
	node->lhs = lhs;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_binary_new(Arena *arena, Location loc, Token_Kind op, Ast_Node *rhs, Ast_Node *lhs)
{
	Ast_Binary *node = ast_node_new(arena, loc, AST_BINARY);
	node->op = op;
	node->rhs = rhs;
	node->lhs = lhs;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_tuple_new(Arena *arena, Location loc, Ast_Node **items, size_t len)
{
	Ast_Tuple *node = ast_node_new(arena, loc, AST_TUPLE);
	node->items = items;
	node->len = len;
	return (Ast_Node *)node;
}

MAC_HOT Ast_Node *
ast_const_new(Arena *arena, Location loc, Const_Value value)
{
	Ast_Const *node = ast_node_new(arena, loc, AST_CONST);
	node->value = value;
	return (Ast_Node *)node;
}
