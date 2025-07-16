#ifndef NML_SYM_H
#define NML_SYM_H

#include "nml_expr.h"
#include "nml_str.h"
#include "nml_tab.h"

typedef enum {
	SYM_GLOB,
	SYM_FUN,
} Sym_Kind;

typedef struct {
	Sym_Kind kind;
	Expr_Node *expr;
} Symbol;

TAB_DECL(sym_tab, Sym, Sized_Str, Symbol)

#endif
