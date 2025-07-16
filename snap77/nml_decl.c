#include "nml_decl.h"
#include "nml_loc.h"
#include "nml_mac.h"
#include "nml_arena.h"
#include "nml_str.h"
#include "nml_type.h"

#define DECL_NEW(_node, _kind, _loc)					\
	arena_alloc(arena, sizeof(_node));					\
	MAC_CAST(Decl_Node *, node)->kind = _kind;			\
	MAC_CAST(Decl_Node *, node)->loc = _loc


MAC_HOT Decl_Node *
decl_let_new(Arena *arena, Location loc, Type hint, Sized_Str name, Expr_Node *value)
{
	Decl_Let *node = DECL_NEW(Decl_Let, DECL_LET, loc);
	node->hint = hint;
	node->name = name;
	node->value = value;
	return (Decl_Node *)node;
}

MAC_HOT Decl_Node *
decl_fun_new(Arena *arena, Location loc, Type hint, Sized_Str name,
			Sized_Str *pars, size_t len, Expr_Node *body)
{
	Decl_Fun *node = DECL_NEW(Decl_Fun, DECL_FUN, loc);
	node->fun = TYPE_NONE;
	node->hint = hint;
	node->name = name;
	node->pars = pars;
	node->len = len;
	node->body = body;
	node->label = 0;
	return (Decl_Node *)node;
}

MAC_HOT Decl_Node *
decl_data_new(Arena *arena, Location loc, Sized_Str name, Type_Scheme scheme,
			struct Decl_Data_Constr *constrs, size_t len)
{
	Decl_Data *node = DECL_NEW(Decl_Data, DECL_DATA, loc);
	node->name = name;
	node->scheme = scheme;
	node->constrs = constrs;
	node->len = len;
	return (Decl_Node *)node;
}

MAC_HOT Decl_Node *
decl_type_new(Arena *arena, Location loc, Sized_Str name, Type type)
{
	Decl_Type *node = DECL_NEW(Decl_Type, DECL_TYPE, loc);
	node->name = name;
	node->type = type;
	return (Decl_Node *)node;
}
