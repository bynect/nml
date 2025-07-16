#include "nml_mod.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_fmt.h"

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
	mod->anon = 1;
	mod->head = NULL;
	mod->tail = NULL;
}

MAC_HOT void
module_fun(Module *mod, Arena *arena, Sized_Str name, Linkage link, Type type, Function fun)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_FUN;
	def->name = name;
	def->link = link;
	def->type = type;
	def->fun = fun;
}

MAC_HOT void
module_var(Module *mod, Arena *arena, Sized_Str name, Linkage link, Type type)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_VAR;
	def->name = name;
	def->type = type;
	def->link = link;
}

MAC_HOT void
module_const(Module *mod, Arena *arena, Sized_Str name, Linkage link, Type type, Const_Value value)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_CONST;
	def->name = name;
	def->link = link;
	def->type = type;
	def->value = value;
}

MAC_HOT Sized_Str
module_fun_anon(Module *mod, Arena *arena, Type type, Function fun)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_FUN;
	def->name = format_str(arena, STR_STATIC("anon.{u}"), mod->anon++);
	def->link = LINK_STAT;
	def->type = type;
	def->fun = fun;
	return def->name;
}

MAC_HOT Sized_Str
module_const_anon(Module *mod, Arena *arena, Type type, Const_Value value)
{
	Definition *def = module_append(mod, arena);
	def->kind = DEF_CONST;
	def->name = format_str(arena, STR_STATIC("anon.{u}"), mod->anon++);
	def->link = LINK_STAT;
	def->type = type;
	def->value = value;
	return def->name;
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
