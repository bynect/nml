#ifndef NML_AST_H
#define NML_AST_H

#include "nml_expr.h"
#include "nml_sig.h"

typedef enum {
	AST_SIG,
	AST_EXPR,
} Ast_Kind;

typedef struct {
	Ast_Kind kind;
	void *node;
} Ast_Node;

typedef struct {
	Ast_Node *items;
	size_t len;
} Ast_Top;

#endif
