#ifndef NML_MOD_H
#define NML_MOD_H

#include "nml_const.h"
#include "nml_ir.h"
#include "nml_fun.h"
#include "nml_mem.h"
#include "nml_type.h"

typedef enum {
	LINK_GLOB,
	LINK_STAT,
	LINK_EXT,
} Linkage;

typedef enum {
	DEF_FUN,
	DEF_VAR,
	DEF_CONST,
} Def_Kind;

typedef struct Definition {
	Def_Kind kind;
	Sized_Str name;
	Linkage link;
	Type type;
	union {
		Function fun;
		Const_Value value;
	};
	struct Definition *next;
} Definition;

typedef struct {
	Sized_Str name;
	uint32_t anon;
	Definition *head;
	Definition *tail;
} Module;

typedef void (*Module_Iter_Fn)(const Definition *def, void *ctx);

void module_init(Module *mod, Sized_Str name);

MAC_HOT void module_fun(Module *mod, Arena *arena, Sized_Str name, Linkage link, Type type, Function fun);

MAC_HOT void module_var(Module *mod, Arena *arena, Sized_Str name, Linkage link, Type type);

MAC_HOT void module_const(Module *mod, Arena *arena, Sized_Str name, Linkage link, Type type, Const_Value value);

MAC_HOT Sized_Str module_fun_anon(Module *mod, Arena *arena, Type type, Function fun);

MAC_HOT Sized_Str module_const_anon(Module *mod, Arena *arena, Type type, Const_Value value);

void module_iter(const Module *mod, Module_Iter_Fn fn, void *ctx);

#endif
