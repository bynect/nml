#ifndef NML_CHECK_H
#define NML_CHECK_H

#include "nml_dbg.h"
#include "nml_set.h"
#include "nml_tab.h"
#include "nml_str.h"
#include "nml_err.h"
#include "nml_mem.h"
#include "nml_buf.h"
#include "nml_ast.h"

TAB_DECL(type_tab, Type, Sized_Str, Type)

TAB_DECL(scheme_tab, Scheme, Sized_Str, Type_Scheme)

typedef struct {
	Hash_Default seed;
	uint32_t tid;
	Arena *arena;
	Error_List *err;
	bool succ;
	const Source *src;
	Scheme_Tab *ctx;
	Str_Set set1;
	Str_Set set2;
} Type_Checker;

void checker_init(Type_Checker *check, Hash_Default seed, Arena *arena, const Source *src, Scheme_Tab *ctx);

void checker_free(Type_Checker *check);

MAC_HOT bool checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err);

#endif
