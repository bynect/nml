#ifndef NML_SIMPL_H
#define NML_SIMPL_H

#include "nml_decl.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_ast.h"

typedef struct {
	Arena *arena;
	Decl_Node **decls;
	size_t len;
	size_t size;
} Simplifier;

void simplifier_init(Simplifier *simpl, Arena *arena);

MAC_HOT void simplifier_simplify(Simplifier *simpl, Ast_Top *ast);

#endif
