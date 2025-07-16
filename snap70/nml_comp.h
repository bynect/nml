#ifndef NML_COMP_H
#define NML_COMP_H

#include "nml_err.h"
#include "nml_expr.h"
#include "nml_hash.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_ast.h"
#include "nml_buf.h"

typedef struct {
	Arena *arena;
	Error_List *err;
	bool succ;
} Compiler;

void compiler_init(Compiler *comp, Arena *arena, Hash_Default seed);

void compiler_free(Compiler *comp);

MAC_HOT bool compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err);

#endif
