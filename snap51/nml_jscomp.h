#ifndef NML_JSCOMP_H
#define NML_JSCOMP_H

#include "nml_ast.h"
#include "nml_mac.h"
#include "nml_err.h"
#include "nml_mem.h"
#include "nml_tab.h"

typedef struct {
	Arena *arena;
	Error_List *err;
	bool succ;
	Hash_Default seed;
	Id_Tab glob;
	uint32_t tmp;
} Js_Compiler;

void jscompiler_init(Js_Compiler *comp, Arena *arena, Error_List *err, Hash_Default seed);

void jscompiler_free(Js_Compiler *comp);

MAC_HOT bool jscompiler_pass(Js_Compiler *comp, Ast_Top *ast);

#endif
