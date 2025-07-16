#ifndef NML_FOLD_H
#define NML_FOLD_H

#include "nml_ast.h"
#include "nml_hash.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_tab.h"

typedef struct {
	Expr_Node *node;
	bool dead;
} Fold_Node;

#define FOLD_NODE(value)	((Fold_Node) { value, true })

TAB_DECL(fold_tab, Fold, Sized_Str, Fold_Node)

MAC_HOT void fold_ast(Arena *arena, Ast_Module *ast, Hash_Default seed);

MAC_HOT void fold_ast_node(Arena *arena, Expr_Node **node, Hash_Default seed);

#endif
