#ifndef NML_BUILD_H
#define NML_BUILD_H

#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_ir.h"
#include "nml_mod.h"
#include "nml_fun.h"

typedef struct {
	Ir_Slot locl;
	Ir_Slot labl;
	Arena *arena;
	Module *mod;
} Builder;

void builder_init(Builder *build, Arena *arena, Module *mod, Ir_Slot args);

MAC_HOT Block *builder_block(Builder *build, Function *fun, Instr *entry);

MAC_HOT Ir_Value builder_unary(Builder *build, Block *block, Instr_Kind kind,
								Ir_Value lhs);

MAC_HOT Ir_Value builder_binary(Builder *build, Block *block, Instr_Kind kind,
								Ir_Value rhs, Ir_Value lhs);

MAC_HOT Ir_Value builder_load(Builder *build, Block *block, Ir_Type type, Ir_Value addr);

MAC_HOT void builder_store(Builder *build, Block *block, Ir_Value addr, Ir_Value value);

MAC_HOT Ir_Value builder_alloca(Builder *build, Block *block, Ir_Type type);

MAC_HOT Ir_Value builder_gep(Builder *build, Block *block, Ir_Type type, Ir_Value addr, Ir_Value index);

MAC_HOT void builder_br(Builder *build, Block *block, Ir_Value labl);

MAC_HOT void builder_cond_br(Builder *build, Block *block, Ir_Value cond,
							Ir_Value b_then, Ir_Value b_else);

MAC_HOT void builder_ret(Builder *build, Block *block, Ir_Value value);

MAC_HOT Ir_Value builder_call(Builder *build, Block *block, Ir_Value fun, Ir_Value *args, size_t len);

static MAC_INLINE inline Ir_Value
builder_const(Builder *build, Ir_Type type, Const_Value value)
{
	return (Ir_Value) {
		.kind = VAR_GLOB,
		.name = module_const_anon(build->mod, build->arena, type, value),
	};
}

static MAC_INLINE inline Ir_Value
builder_local(Builder *build, Ir_Type type)
{
	return ir_local(type, build->locl++);
}

static MAC_INLINE inline Ir_Value
builder_label(Builder *build)
{
	return ir_label(build->labl++);
}

#endif
