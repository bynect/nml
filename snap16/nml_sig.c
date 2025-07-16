#include "nml_sig.h"
#include "nml_dbg.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"

static const size_t sizes[] = {
#define NODE(kind, node, _) [SIG_ ## kind] = sizeof(Sig_ ## node),
#define SIG
#include "nml_node.h"
#undef SIG
#undef NODE
};

static MAC_INLINE inline void *
sig_node_new(Arena *arena, Location loc, Sig_Kind kind)
{
	Sig_Node *node = arena_alloc(arena, sizes[kind]);
	node->kind = kind;
	node->loc = loc;
	return (void *)node;
}

MAC_HOT Sig_Node *
sig_type_new(Arena *arena, Location loc, Sized_Str name, Type type)
{
	Sig_Type *node = sig_node_new(arena, loc, SIG_TYPE);
	node->name = name;
	node->type = type;
	return (Sig_Node *)node;
}
