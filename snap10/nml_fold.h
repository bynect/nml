#ifndef NML_FOLD_H
#define NML_FOLD_H

#include "nml_ast.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_hash.h"
#include "nml_type.h"

typedef struct {
	Arena *arena;
	Hash_Default seed;
	Error_List *err;
} Folder;

void folder_init(Folder *fold, Arena *arena, Hash_Default seed);

MAC_HOT void folder_ast(Folder *fold, Ast_Top *ast, Error_List *err);

MAC_HOT void folder_ast_node(Folder *fold, Ast_Node **node, Error_List *err);

#endif
