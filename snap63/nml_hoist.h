#ifndef NML_HOIST_H
#define NML_HOIST_H

#include "nml_ast.h"
#include "nml_decl.h"
#include "nml_err.h"
#include "nml_mem.h"
#include "nml_str.h"

typedef struct {
	Arena *arena;
	uint32_t label;
	Decl_Node **decls;
	size_t len;
	size_t size;
} Hoister;

void hoister_init(Hoister *hoist, Arena *arena);

MAC_HOT void hoister_pass(Hoister *hoist, Ast_Top *ast);

#endif
