#ifndef NML_PATN_H
#define NML_PATN_H

#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_type.h"
#include "nml_err.h"
#include "nml_lit.h"
#include "nml_dbg.h"
#include "nml_expr.h"

typedef enum {
#define NODE(kind, node, _) PATN_ ## kind,
#define PATN
#include "nml_node.h"
#undef PATN
#undef NODE
} Patn_Kind;

typedef struct Patn_Node {
	Patn_Kind kind;
	Type type;
	Location loc;
} Patn_Node;

typedef struct {
	Patn_Node node;
	Sized_Str name;
} Patn_Ident;

typedef struct {
	Patn_Node node;
	Sized_Str name;
	Patn_Node *patn;
} Patn_Constr;

typedef struct {
	Patn_Node node;
	Sized_Str name;
	Patn_Node *patn;
} Patn_As;

typedef struct {
	Patn_Node node;
	Patn_Node **items;
	size_t len;
} Patn_Tuple;

typedef struct {
	Patn_Node node;
	Lit_Value value;
} Patn_Lit;

Patn_Node *patn_ident_new(Arena *arena, Location loc, Sized_Str name);

Patn_Node *patn_constr_new(Arena *arena, Location loc, Sized_Str name, Patn_Node *patn);

Patn_Node *patn_as_new(Arena *arena, Location loc, Sized_Str name, Patn_Node *patn);

Patn_Node *patn_tuple_new(Arena *arena, Location loc, Patn_Node **items, size_t len);

Patn_Node *patn_lit_new(Arena *arena, Location loc, Lit_Value value);

MAC_CONST bool patn_refutable(const Patn_Node *node);

MAC_HOT bool patn_check(const Patn_Node *node, Arena *arena, Str_Set *set, Error_List *err);

MAC_HOT bool patn_match_check(const Expr_Match *node, Arena *arena, Str_Set *set, Error_List *err);

#endif
