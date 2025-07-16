#include "nml_decl.h"
#include "nml_dbg.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"

static const size_t sizes[] = {
#define NODE(kind, node, _) [DECL_ ## kind] = sizeof(Decl_ ## node),
#define DECL
#include "nml_node.h"
#undef DECL
#undef NODE
};

static MAC_INLINE inline void *
decl_node_new(Arena *arena, Location loc, Decl_Kind kind)
{
	Decl_Node *node = arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->loc = loc;
	return (void *)node;
}

MAC_HOT Decl_Node *
decl_type_new(Arena *arena, Location loc, Sized_Str name, Type type)
{
	Decl_Type *node = decl_node_new(arena, loc, DECL_TYPE);
	node->name = name;
	node->type = type;
	return (Decl_Node *)node;
}

MAC_HOT Decl_Node *
decl_let_new(Arena *arena, Location loc, Type hint, Sized_Str name, Expr_Node *value)
{
	Decl_Let *node = decl_node_new(arena, loc, DECL_LET);
	node->hint = hint;
	node->name = name;
	node->value = value;
	return (Decl_Node *)node;
}

MAC_HOT Decl_Node *
decl_fun_new(Arena *arena, Location loc, Type hint, Sized_Str name,
			Sized_Str *pars, size_t len, Expr_Node *body)
{
	Decl_Fun *node = decl_node_new(arena, loc, DECL_FUN);
	node->hint = hint;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	return (Decl_Node *)node;
}
