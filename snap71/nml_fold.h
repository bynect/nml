#ifndef NML_FOLD_H
#define NML_FOLD_H

#include "nml_ast.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_expr.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_hash.h"
#include "nml_type.h"

typedef struct {
	Arena *arena;
	Error_List *err;
	Hash_Default seed;
	Expr_Tab *def;
} Folder;

void folder_init(Folder *fold, Arena *arena, Hash_Default seed, Expr_Tab *def);

MAC_HOT void folder_pass(Folder *fold, Ast_Top *ast, Error_List *err);

#endif
