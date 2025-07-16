#include <stddef.h>
#include <stdint.h>

#include "nml_ast.h"
#include "nml_parse.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_type.h"

static const size_t sizes[] = {
#define NODE(kind, node) [AST_ ## kind] = sizeof(Ast_ ## node),
#include "nml_node.h"
#undef NODE
};

static MAC_INLINE inline void *
ast_node_new(Arena *arena, Ast_Kind kind)
{
	Ast_Node *node =  arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->type = TYPE_NONE;
	return (void *)node;
}

Ast_Node *
ast_if_new(Arena *arena, Ast_Node *cond, Ast_Node *then, Ast_Node *m_else)
{
	Ast_If *node = ast_node_new(arena, AST_IF);
	node->cond = cond;
	node->then = then;
	node->m_else = m_else;
	return (Ast_Node *)node;
}

Ast_Node *
ast_let_new(Arena *arena, Sized_Str name, Ast_Node *value, Ast_Node *expr)
{
	Ast_Let *node = ast_node_new(arena, AST_LET);
	node->name = name;
	node->bind = TYPE_NONE;
	node->value = value;
	node->expr = expr;
	return (Ast_Node *)node;
}

Ast_Node *
ast_fun_new(Arena *arena, const Sized_Str name, Sized_Str *pars,
			size_t len, Ast_Node *body, Ast_Node *expr)
{
	Ast_Fun *node = ast_node_new(arena, AST_FUN);
	node->fun = TYPE_NONE;
	node->rec = TYPE_NONE;
	node->ret = TYPE_NONE;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->expr = expr;
	return (Ast_Node *)node;
}

Ast_Node *
ast_app_new(Arena *arena, Ast_Node *fun, Ast_Node **args, size_t len)
{
	Ast_App *node = ast_node_new(arena, AST_APP);
	node->fun = fun;
	node->args = args;
	node->len = len;
	return (Ast_Node *)node;
}

Ast_Node *
ast_ident_new(Arena *arena, const Sized_Str name)
{
	Ast_Ident *node = ast_node_new(arena, AST_IDENT);
	node->name = name;
	return (Ast_Node *)node;
}

Ast_Node *
ast_unary_new(Arena *arena, Token_Kind op, Ast_Node *lhs)
{
	Ast_Unary *node = ast_node_new(arena, AST_UNARY);
	node->op = op;
	node->lhs = lhs;
	return (Ast_Node *)node;
}

Ast_Node *
ast_binary_new(Arena *arena, Token_Kind op, Ast_Node *rhs, Ast_Node *lhs)
{
	Ast_Binary *node = ast_node_new(arena, AST_BINARY);
	node->op = op;
	node->rhs = rhs;
	node->lhs = lhs;
	return (Ast_Node *)node;
}

Ast_Node *
ast_tuple_new(Arena *arena, Ast_Node **items, size_t len)
{
	Ast_Tuple *node = ast_node_new(arena, AST_TUPLE);
	node->items = items;
	node->len = len;
	return (Ast_Node *)node;
}

Ast_Node *
ast_const_new(Arena *arena, Const_Kind kind, const void *value)
{
	Ast_Const *node = ast_node_new(arena, AST_CONST);
	node->kind = kind;

	switch (kind)
	{
		case CONST_UNIT:
			node->node.type = TYPE_UNIT;
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

	return (Ast_Node *)node;
}

Ast_Node *
ast_type_new(Arena *arena, Sized_Str name, Sized_Str *vars, Type *types, size_t len)
{
	Ast_Type *node = ast_node_new(arena, AST_TYPE);
	node->name = name;
	node->vars = vars;
	node->types = types;
	node->len = len;
	return (Ast_Node *)node;
}
