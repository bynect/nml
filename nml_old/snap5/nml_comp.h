#ifndef NML_COMP_H
#define NML_COMP_H

#include "nml_ast.h"
#include "nml_err.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_box.h"

typedef struct {
	Arena *arena;
	Error_List *err;
	bool succ;
} Compiler;

void compiler_init(Compiler *comp, Arena *arena);

MAC_HOT bool compiler_compile(Compiler *comp, Ast_Module *ast, Error_List *err);

#endif
