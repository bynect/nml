#ifndef NML_AST_H
#define NML_AST_H

#include "nml_expr.h"
#include "nml_decl.h"

typedef struct {
	Decl_Node **decls;
	size_t len;
} Ast_Top;

#endif
