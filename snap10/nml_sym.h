#ifndef NML_SYM_H
#define NML_SYM_H

#include "nml_const.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_ast.h"
#include "nml_type.h"

typedef enum {
	SYM_FUN,
	SYM_VAR,
	SYM_CONST,
} Sym_Kind;

typedef struct {
	Sym_Kind kind;
	union {
		Const_Value value;
	};
} Symbol;

TAB_DECL(sym_tab, Sym, Sized_Str, Symbol)

#endif
