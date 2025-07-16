#ifndef NML_SYM_H
#define NML_SYM_H

#include "nml_str.h"
#include "nml_tab.h"
#include "nml_ast.h"
#include "nml_type.h"

typedef struct {
	Ast_Node *node;
} Symbol;

TAB_DECL(sym_tab, Sym, Sized_Str, Symbol)

#endif
