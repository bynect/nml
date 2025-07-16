#ifndef NML_AST_H
#define NML_AST_H

#include "nml_expr.h"
#include "nml_sig.h"

typedef struct {
	void *node;
	bool sig;
} Ast_Node;

typedef struct {
	Ast_Node *items;
	size_t len;
} Ast_Top;

#endif
