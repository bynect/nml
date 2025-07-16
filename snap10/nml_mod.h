#ifndef NML_MOD_H
#define NML_MOD_H

#include "nml_ir.h"
#include "nml_type.h"

typedef enum {
	DEF_FUN,
	DEF_VAR,
	DEF_CONST,
} Def_Kind;

typedef struct Definition {
	Def_Kind kind;
	Sized_Str name;
	Type type;
	union {
		Function *fun;
		Const_Value value;
	};
	bool glob;
	struct Definition *next;
} Definition;

typedef struct {
	Sized_Str name;
	Definition *head;
	Definition *tail;
} Module;

typedef void (*Module_Iter_Fn)(const Definition *def, void *ctx);

void module_init(Module *mod, Sized_Str name);

MAC_HOT void module_fun(Module *mod, Arena *arena, Sized_Str name, Function *fun, bool glob);

MAC_HOT void module_var(Module *mod, Arena *arena, Sized_Str name, bool glob);

MAC_HOT void module_const(Module *mod, Arena *arena, Sized_Str name, Const_Value value, bool glob);

void module_iter(const Module *mod, Module_Iter_Fn fn, void *ctx);

#endif
