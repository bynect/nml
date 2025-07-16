#include "nml_mod.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_sym.h"

static MAC_INLINE inline Definition *
module_append(Module *mod, Arena *arena)
{
	if (MAC_UNLIKELY(mod->head == NULL))
	{
		mod->head = arena_alloc(arena, sizeof(Definition));
		mod->tail = mod->head;
	}
	else
	{
		mod->tail->next = arena_alloc(arena, sizeof(Definition));
		mod->tail = mod->tail->next;
	}

	mod->tail->next = NULL;
	return mod->tail;
}

void
module_init(Module *mod, Sized_Str name)
{
	mod->name = name;
	mod->head = NULL;
	mod->tail = NULL;
}

MAC_HOT void
module_fun(Module *mod, Arena *arena, Sized_Str name, Function *fun, bool glob)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_FUN;
	def->name = name;
	def->fun = fun;
	def->glob = glob;
}

MAC_HOT void
module_var(Module *mod, Arena *arena, Sized_Str name, bool glob)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_VAR;
	def->name = name;
	def->glob = glob;
}

MAC_HOT void
module_const(Module *mod, Arena *arena, Sized_Str name, Const_Value value, bool glob)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_CONST;
	def->name = name;
	def->value = value;
	def->glob = glob;
}

void
module_iter(const Module *mod, Module_Iter_Fn fn, void *ctx)
{
	Definition *ptr = mod->head;
	while (ptr != NULL)
	{
		fn(ptr, ctx);
		ptr = ptr->next;
	}
}
