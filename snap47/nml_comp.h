#ifndef NML_COMP_H
#define NML_COMP_H

#include "nml_err.h"
#include "nml_hash.h"
#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_ast.h"
#include "nml_buf.h"

typedef uint32_t Var_Id;

TAB_DECL(id_tab, Id, Sized_Str, Var_Id)

typedef struct {
	Arena *arena;
	Error_List *err;
	bool succ;
	Var_Id id;
	Id_Tab tab;
} Compiler;

void compiler_init(Compiler *comp, Arena *arena, Hash_Default seed);

void compiler_free(Compiler *comp);

MAC_HOT bool compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err);

//typedef struct {
//	Arena *arena;
//	Module *mod;
//	Error_List *err;
//	bool succ;
//	Hash_Default seed;
//	Builder build;
//} Compiler;
//
//TAB_DECL(value_tab, Value, Sized_Str, Ir_Value)
//
//void compiler_init(Compiler *comp, Arena *arena, Module *mod, Hash_Default seed);
//
//MAC_HOT bool compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err);

#endif
