#ifndef NML_COMP_H
#define NML_COMP_H

#include "nml_ast.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_sym.h"

typedef struct {
	Arena *arena;
	Error_List *err;
	bool succ;
	Hash_Default seed;
	Sym_Tab tab;
} Compiler;

void compiler_init(Compiler *comp, Arena *arena, Hash_Default seed);

MAC_HOT bool compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err);

#endif
