#include "nml_build.h"
#include "nml_const.h"
#include "nml_fun.h"
#include "nml_ir.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_mod.h"
#include "nml_type.h"

void
builder_init(Builder *build, Arena *arena, Module *mod)
{
	build->var = 1;
	build->addr = 1;
	build->block = 1;
	build->arena = arena;
	build->mod = mod;
}

void
builder_set(Builder *build, Var_Id var, Var_Id addr, Block_Id block)
{
	build->var = var;
	build->addr = addr;
	build->block = block;
}

MAC_HOT Block *
builder_block(Builder *build, Function *fun, Instr *entry)
{
	Block *block = arena_alloc(build->arena, sizeof(Block));
	block_init(block, build->block++, entry);
	function_append(fun, block);
	return block;
}

MAC_HOT Variable
builder_unary(Builder *build, Block *block, Instr_Kind kind,
				Type type, Variable lhs)
{
	Variable yield = builder_local(build);
	Instr *instr = instr_unary_new(build->arena, kind, type, lhs, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT Variable
builder_binary(Builder *build, Block *block, Instr_Kind kind,
				Type type, Variable rhs, Variable lhs)
{
	Variable yield = builder_local(build);
	Instr *instr = instr_binary_new(build->arena, kind, type, rhs, lhs, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT Variable
builder_load(Builder *build, Block *block, Type type, Variable addr)
{
	Variable yield = builder_local(build);
	Instr *instr = instr_load_new(build->arena, type, addr, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT void
builder_store(Builder *build, Block *block, Type type, Variable addr, Variable value)
{
	Instr *instr = instr_store_new(build->arena, type, addr, value);
	block_append(block, instr);
}

MAC_HOT Variable
builder_alloc(Builder *build, Block *block, Type type)
{
	Variable yield = builder_address(build);
	Instr *instr = instr_alloc_new(build->arena, type, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT Variable
builder_elemof(Builder *build, Block *block, Type type, Variable addr, uint32_t index)
{
	Variable yield = builder_address(build);
	Instr *instr = instr_elemof_new(build->arena, type, addr, index, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT Variable
builder_addrof(Builder *build, Block *block, Type type, Variable value)
{
	Variable yield = builder_address(build);
	Instr *instr = instr_addrof_new(build->arena, type, value, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT void
builder_br(Builder *build, Block *block, Block_Id target)
{
	Instr *instr = instr_br_new(build->arena, TYPE_NONE, VAR_ERR, target, BLOCK_ERR);
	block_append(block, instr);
}

MAC_HOT void
builder_cond_br(Builder *build, Block *block, Variable cond,
				Block_Id then, Block_Id m_else)
{
	Instr *instr = instr_br_new(build->arena, TYPE_NONE, cond, then, m_else);
	block_append(block, instr);
}

MAC_HOT void
builder_ret(Builder *build, Block *block, Variable value)
{
	Instr *instr = instr_ret_new(build->arena, TYPE_NONE, value);
	block_append(block, instr);
}

MAC_HOT Variable
builder_call(Builder *build, Block *block, Type type, Variable fun, Variable *args, size_t len)
{
	Variable yield = builder_local(build);
	Instr *instr = instr_call_new(build->arena, type, fun, args, len, yield);
	block_append(block, instr);
	return yield;
}

MAC_HOT Variable
builder_const(Builder *build, Type type, Const_Value value)
{
	return (Variable) {
		.kind = VAR_GLOB,
		.name = module_const_anon(build->mod, build->arena, type, value),
	};
}

MAC_HOT Variable
builder_local(Builder *build)
{
	return (Variable) {
		.kind = VAR_LOC,
		.id = build->var++,
	};
}

MAC_HOT Variable
builder_address(Builder *build)
{
	return (Variable) {
		.kind = VAR_ADDR,
		.id = build->addr++,
	};
}
