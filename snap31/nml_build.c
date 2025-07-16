#include "nml_build.h"
#include "nml_const.h"
#include "nml_fun.h"
#include "nml_ir.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_mod.h"
#include "nml_type.h"

//void
//builder_init(Builder *build, Arena *arena, Module *mod, Ir_Slot args)
//{
//	build->locl = args;
//	build->labl = 0;
//	build->arena = arena;
//	build->mod = mod;
//}
//
//MAC_HOT Block *
//builder_block(Builder *build, Function *fun, Instr *entry)
//{
//	Block *block = arena_alloc(build->arena, sizeof(Block));
//	block_init(block, build->labl++, entry);
//	function_append(fun, block);
//	return block;
//}
//
//MAC_HOT Ir_Value
//builder_unary(Builder *build, Block *block, Instr_Kind kind, Ir_Value lhs)
//{
//	Ir_Value yield = builder_local(build, lhs.type);
//	Instr *instr = instr_unary_new(build->arena, kind, lhs, yield);
//	block_append(block, instr);
//	return yield;
//}
//
//MAC_HOT Ir_Value
//builder_binary(Builder *build, Block *block, Instr_Kind kind,
//				Ir_Value rhs, Ir_Value lhs)
//{
//	Ir_Value yield = builder_local(build, lhs.type);
//	Instr *instr = instr_binary_new(build->arena, kind, rhs, lhs, yield);
//	block_append(block, instr);
//	return yield;
//}
//
//MAC_HOT Ir_Value
//builder_load(Builder *build, Block *block, Ir_Type type, Ir_Value addr)
//{
//	Ir_Value yield = builder_local(build, type);
//	Instr *instr = instr_load_new(build->arena, type, addr, yield);
//	block_append(block, instr);
//	return yield;
//}
//
//MAC_HOT void
//builder_store(Builder *build, Block *block, Ir_Value addr, Ir_Value value)
//{
//	Instr *instr = instr_store_new(build->arena, addr, value);
//	block_append(block, instr);
//}
//
//MAC_HOT Ir_Value
//builder_alloca(Builder *build, Block *block, Ir_Type type)
//{
//	Ir_Type ptr = ir_type_ptr(ir_type_copy(build->arena, type));
//	Ir_Value yield = builder_local(build, ptr);
//	Instr *instr = instr_alloca_new(build->arena, type, yield);
//	block_append(block, instr);
//	return yield;
//}
//
//MAC_HOT Ir_Value
//builder_gep(Builder *build, Block *block, Ir_Type type, Ir_Value addr, Ir_Value index)
//{
//	Ir_Type ptr = ir_type_ptr(ir_type_copy(build->arena, type));
//	Ir_Value yield = builder_local(build, ptr);
//	Instr *instr = instr_gep_new(build->arena, type, addr, index, yield);
//	block_append(block, instr);
//	return yield;
//}
//
//MAC_HOT void
//builder_br(Builder *build, Block *block, Ir_Value labl)
//{
//	Instr *instr = instr_br_new(build->arena, labl);
//	block_append(block, instr);
//}
//
//MAC_HOT void
//builder_cond_br(Builder *build, Block *block, Ir_Value cond,
//				Ir_Value b_then, Ir_Value b_else)
//{
//	Instr *instr = instr_condbr_new(build->arena, cond, b_then, b_else);
//	block_append(block, instr);
//}
//
//MAC_HOT void
//builder_ret(Builder *build, Block *block, Ir_Value value)
//{
//	Instr *instr = instr_ret_new(build->arena, value);
//	block_append(block, instr);
//}
//
//MAC_HOT Ir_Value
//builder_call(Builder *build, Block *block, Ir_Value fun, Ir_Value *args, size_t len)
//{
//	Ir_Type type = *fun.type.fun.ret;
//	Ir_Value yield = builder_local(build, type);
//	Instr *instr = instr_call_new(build->arena, fun, args, len, yield);
//	block_append(block, instr);
//	return yield;
//}
