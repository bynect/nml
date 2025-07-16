#ifndef NML_MERGE_H
#define NML_MERGE_H

#include "nml_ast.h"
#include "nml_mac.h"
#include "nml_arena.h"

typedef struct {
	Arena *arena;
	bool lambda_chain;
	bool app_chain;
} Merger;

void merger_init(Merger *merge, Arena *arena);

MAC_HOT void merger_pass(Merger *merge, Ast_Top *ast);

#endif
