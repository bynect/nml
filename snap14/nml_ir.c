#include "nml_ir.h"
#include "nml_mem.h"

MAC_HOT Instr *
instr_unary_new(Arena *arena, Instr_Kind kind, Type type, Variable lhs, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = kind;
	instr->type = type;
	instr->unary.lhs = lhs;
	instr->unary.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_binary_new(Arena *arena, Instr_Kind kind, Type type, Variable rhs,
				Variable lhs, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = kind;
	instr->type = type;
	instr->binary.rhs = rhs;
	instr->binary.lhs = lhs;
	instr->binary.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_load_new(Arena *arena, Type type, Variable addr, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_LOAD;
	instr->type = type;
	instr->load.addr = addr;
	instr->load.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_store_new(Arena *arena, Type type, Variable addr, Variable value)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_STORE;
	instr->type = type;
	instr->store.addr = addr;
	instr->store.value = value;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_alloc_new(Arena *arena, Type type, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_ALLOCA;
	instr->type = type;
	instr->alloc.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_elemof_new(Arena *arena, Type type, Variable addr,
				uint32_t index, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_ELEMOF;
	instr->type = type;
	instr->elemof.addr = addr;
	instr->elemof.index = index;
	instr->elemof.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_addrof_new(Arena *arena, Type type, Variable value, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_ADDROF;
	instr->type = type;
	instr->addrof.value = value;
	instr->addrof.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_br_new(Arena *arena, Type type, Variable cond,
				Block_Id b_then, Block_Id b_else)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_BR;
	instr->type = type;
	instr->br.cond = cond;
	instr->br.b_then = b_then;
	instr->br.b_else = b_else;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_ret_new(Arena *arena, Type type, Variable value)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_RET;
	instr->type = type;
	instr->ret.value = value;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_call_new(Arena *arena, Type type, Variable fun,
				Variable *args, size_t len, Variable yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_CALL;
	instr->type = type;
	instr->call.fun = fun;
	instr->call.args = args;
	instr->call.len = len;
	instr->call.yield = yield;
	instr->next = NULL;
	return instr;
}
