#ifndef NML_SIG_H
#define NML_SIG_H

#include "nml_dbg.h"
#include "nml_str.h"
#include "nml_type.h"

typedef enum {
#define NODE(kind, node, _) SIG_ ## kind,
#define SIG
#include "nml_node.h"
#undef SIG
#undef NODE
} Sig_Kind;

typedef struct Sig_Node {
	Sig_Kind kind;
	Location loc;
} Sig_Node;

typedef struct {
	Sig_Node node;
	Sized_Str name;
	Type type;
} Sig_Type;

MAC_HOT Sig_Node *sig_type_new(Arena *arena, Location loc, Sized_Str name, Type type);

#endif
