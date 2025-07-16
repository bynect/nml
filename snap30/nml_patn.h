#ifndef NML_PATN_H
#define NML_PATN_H

#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_type.h"
#include "nml_err.h"
#include "nml_const.h"
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
	Patn_Node **items;
	size_t len;
} Patn_Tuple;

typedef struct {
	Patn_Node node;
	Const_Value value;
} Patn_Const;

Patn_Node *patn_ident_new(Arena *arena, Location loc, Sized_Str name);

Patn_Node *patn_tuple_new(Arena *arena, Location loc, Patn_Node **items, size_t len);

Patn_Node *patn_const_new(Arena *arena, Location loc, Const_Value value);

MAC_CONST bool pattern_refutable(const Patn_Node *node);

MAC_HOT bool pattern_check(const Patn_Node *node, Arena *arena, Str_Set *set, Error_List *err);

MAC_HOT bool pattern_match_check(const Expr_Match *node, Arena *arena, Hash_Default seed, Error_List *err);

#endif
