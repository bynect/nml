#ifndef NML_CHECK_H
#define NML_CHECK_H

#include "nml_type.h"
#include "nml_tab.h"
#include "nml_str.h"
#include "nml_err.h"
#include "nml_mem.h"
#include "nml_buf.h"
#include "nml_ast.h"

typedef struct {
	Type t1;
	Type t2;
} Type_Constr;

#define TYPE_CONSTR(t1, t2)		((Type_Constr) {(t1), (t2)})

TAB_DECL(type_tab, Type, Sized_Str, Type)

TAB_DECL(subst_tab, Subst, Type_Id, Type)

BUF_DECL(constr_buf, Constr, Type_Constr)

typedef struct {
	Hash_Default seed;
	Type_Id tid;
	Arena *arena;
	Error_List *err;
	bool succ;
	Type_Tab env;
	Type_Tab *ctx;
} Type_Checker;

void checker_init(Type_Checker *check, Arena *arena, Hash_Default seed, Type_Tab *ctx);

MAC_HOT bool checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err);

#endif
