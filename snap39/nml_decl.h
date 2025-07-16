#ifndef NML_DECL_H
#define NML_DECL_H

#include "nml_dbg.h"
#include "nml_str.h"
#include "nml_type.h"
#include "nml_expr.h"

typedef enum {
#define NODE(kind, node, _) DECL_ ## kind,
#define DECL
#include "nml_node.h"
#undef DECL
#undef NODE
} Decl_Kind;

typedef struct Decl_Node {
	Decl_Kind kind;
	Location loc;
} Decl_Node;

typedef struct {
	Decl_Node node;
	Type hint;
	Sized_Str name;
	Expr_Node *value;
} Decl_Let;

typedef struct {
	Decl_Node node;
	Type fun;
	Type hint;
	Sized_Str name;
	Sized_Str *pars;
	size_t len;
	Expr_Node *body;
} Decl_Fun;

typedef struct {
	Decl_Node node;
	Sized_Str name;
	Type type;
} Decl_Type;

MAC_HOT Decl_Node *decl_let_new(Arena *arena, Location loc, Type hint, Sized_Str name, Expr_Node *value);

MAC_HOT Decl_Node *decl_fun_new(Arena *arena, Location loc, Type hint, Sized_Str name,
								Sized_Str *pars, size_t len, Expr_Node *body);

MAC_HOT Decl_Node *decl_type_new(Arena *arena, Location loc, Sized_Str name, Type type);

#endif
