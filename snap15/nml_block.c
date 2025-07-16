#include "nml_block.h"

void
block_init(Block *block, Ir_Slot id, Instr *entry)
{
	block->labl = id;
	block->head = entry;
	block->tail = block->head;
	block->next = NULL;
}

MAC_HOT void
block_append(Block *block, Instr *instr)
{
	if (MAC_UNLIKELY(block->head == NULL))
	{
		block->head = instr;
		block->tail = block->head;
	}
	else
	{
		block->tail->next = instr;
		block->tail = instr;
	}
}

MAC_HOT Instr *
instr_unary_new(Arena *arena, Instr_Kind kind, Ir_Value lhs, Ir_Value yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = kind;
	instr->unary.lhs = lhs;
	instr->unary.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_binary_new(Arena *arena, Instr_Kind kind, Ir_Value rhs,
				Ir_Value lhs, Ir_Value yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = kind;
	instr->binary.rhs = rhs;
	instr->binary.lhs = lhs;
	instr->binary.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_load_new(Arena *arena, Ir_Type type, Ir_Value addr, Ir_Value yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_LOAD;
	instr->load.type = type;
	instr->load.addr = addr;
	instr->load.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_store_new(Arena *arena, Ir_Value addr, Ir_Value value)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_STORE;
	instr->store.addr = addr;
	instr->store.value = value;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_alloca_new(Arena *arena, Ir_Type type, Ir_Value yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_ALLOCA;
	instr->alloca.type = type;
	instr->alloca.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_gep_new(Arena *arena, Ir_Type type, Ir_Value addr,
				Ir_Value index, Ir_Value yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_GEP;
	instr->gep.type = type;
	instr->gep.addr = addr;
	instr->gep.index = index;
	instr->gep.yield = yield;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_br_new(Arena *arena, Ir_Slot labl)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_BR;
	instr->br.labl = labl;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_condbr_new(Arena *arena, Ir_Value cond, Ir_Slot b_then, Ir_Slot b_else)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_CONDBR;
	instr->condbr.cond = cond;
	instr->condbr.b_then = b_then;
	instr->condbr.b_else = b_else;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_ret_new(Arena *arena, Ir_Value value)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_RET;
	instr->ret.value = value;
	instr->next = NULL;
	return instr;
}

MAC_HOT Instr *
instr_call_new(Arena *arena, Ir_Value fun, Ir_Value *args,
				size_t len, Ir_Value yield)
{
	Instr *instr = arena_alloc(arena, sizeof(Instr));
	instr->kind = INSTR_CALL;
	instr->call.fun = fun;
	instr->call.args = args;
	instr->call.len = len;
	instr->call.yield = yield;
	instr->next = NULL;
	return instr;
}
