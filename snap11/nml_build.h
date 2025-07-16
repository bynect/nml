#ifndef NML_BUILD_H
#define NML_BUILD_H

#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_ir.h"
#include "nml_mod.h"
#include "nml_fun.h"

typedef struct {
	Var_Id var;
	Var_Id addr;
	Block_Id block;
	Arena *arena;
	Module *mod;
} Builder;

void builder_init(Builder *build, Arena *arena, Module *mod);

void builder_set(Builder *build, Var_Id var, Var_Id addr, Block_Id block);

MAC_HOT Block *builder_block(Builder *build, Function *fun, Instr *entry);

MAC_HOT Variable builder_unary(Builder *build, Block *block, Instr_Kind kind,
								Type type, Variable lhs);

MAC_HOT Variable builder_binary(Builder *build, Block *block, Instr_Kind kind,
								Type type, Variable rhs, Variable lhs);

MAC_HOT Variable builder_load(Builder *build, Block *block, Type type, Variable addr);

MAC_HOT void builder_store(Builder *build, Block *block, Type type,
						Variable addr, Variable value);

MAC_HOT Variable builder_alloc(Builder *build, Block *block, Type type);

MAC_HOT Variable builder_elemof(Builder *build, Block *block, Type type,
								Variable addr, uint32_t index);

MAC_HOT Variable builder_addrof(Builder *build, Block *block, Type type, Variable value);

MAC_HOT void builder_br(Builder *build, Block *block, Block_Id target);

MAC_HOT void builder_cond_br(Builder *build, Block *block, Variable cond,
							Block_Id then, Block_Id m_else);

MAC_HOT void builder_ret(Builder *build, Block *block, Variable value);

MAC_HOT Variable builder_call(Builder *build, Block *block, Type type,
								Variable fun, Variable *args, size_t len);

MAC_HOT Variable builder_const(Builder *build, Type type, Const_Value value);

MAC_HOT Variable builder_local(Builder *build);

MAC_HOT Variable builder_address(Builder *build);

#endif
