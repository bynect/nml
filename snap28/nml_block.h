#ifndef NML_BLOCK_H
#define NML_BLOCK_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_ir.h"

typedef struct Block {
	Ir_Slot labl;
	struct Instr *head;
	struct Instr *tail;
	struct Block *next;
} Block;

typedef enum {
#define INSTR(name, _) INSTR_ ## name,
#include "nml_instr.h"
#undef INSTR
} Instr_Kind;

typedef struct {
	Ir_Value lhs;
	Ir_Value yield;
} Instr_Unary;

typedef struct {
	Ir_Value rhs;
	Ir_Value lhs;
	Ir_Value yield;
} Instr_Binary;

typedef struct {
	Ir_Type type;
	Ir_Value addr;
	Ir_Value yield;
} Instr_Load;

typedef struct {
	Ir_Value addr;
	Ir_Value value;
} Instr_Store;

typedef struct {
	Ir_Type type;
	Ir_Value yield;
} Instr_Alloca;

typedef struct {
	Ir_Type type;
	Ir_Value addr;
	Ir_Value index;
	Ir_Value yield;
} Instr_Gep;

typedef struct {
	Ir_Value labl;
} Instr_Br;

typedef struct {
	Ir_Value cond;
	Ir_Value b_then;
	Ir_Value b_else;
} Instr_Condbr;

typedef struct {
	Ir_Value value;
} Instr_Ret;

typedef struct {
	Ir_Value fun;
	Ir_Value *args;
	size_t len;
	Ir_Value yield;
} Instr_Call;

typedef struct Instr {
	Instr_Kind kind;
	union {
		Instr_Unary unary;
		Instr_Binary binary;
		Instr_Load load;
		Instr_Store store;
		Instr_Alloca alloca;
		Instr_Gep gep;
		Instr_Br br;
		Instr_Condbr condbr;
		Instr_Ret ret;
		Instr_Call call;
	};
	struct Instr *next;
} Instr;

void block_init(Block *block, Ir_Slot id, Instr *entry);

MAC_HOT void block_append(Block *block, Instr *instr);

MAC_HOT Instr *instr_unary_new(Arena *arena, Instr_Kind kind,
							Ir_Value lhs, Ir_Value yield);

MAC_HOT Instr *instr_binary_new(Arena *arena, Instr_Kind kind,
								Ir_Value rhs, Ir_Value lhs, Ir_Value yield);

MAC_HOT Instr *instr_load_new(Arena *arena, Ir_Type type,
								Ir_Value addr, Ir_Value yield);

MAC_HOT Instr *instr_store_new(Arena *arena, Ir_Value addr, Ir_Value value);

MAC_HOT Instr *instr_alloca_new(Arena *arena, Ir_Type type, Ir_Value yield);

MAC_HOT Instr *instr_gep_new(Arena *arena, Ir_Type type, Ir_Value addr,
							Ir_Value index, Ir_Value yield);

MAC_HOT Instr *instr_br_new(Arena *arena, Ir_Value labl);

MAC_HOT Instr *instr_condbr_new(Arena *arena, Ir_Value cond,
								Ir_Value b_then, Ir_Value b_else);

MAC_HOT Instr *instr_ret_new(Arena *arena, Ir_Value value);

MAC_HOT Instr *instr_call_new(Arena *arena, Ir_Value fun, Ir_Value *args,
								size_t len, Ir_Value yield);

static MAC_INLINE inline bool
instr_int(Instr *instr)
{
	return instr->kind >= INSTR_INEG && instr->kind <= INSTR_XOR;
}

static MAC_INLINE inline bool
instr_float(Instr *instr)
{
	return instr->kind >= INSTR_FNEG && instr->kind <= INSTR_FNE;
}

static MAC_INLINE inline bool
instr_bit(Instr *instr)
{
	return instr->kind >= INSTR_NOT && instr->kind <= INSTR_XOR;
}

static MAC_INLINE inline bool
instr_mem(Instr *instr)
{
	return instr->kind >= INSTR_LOAD && instr->kind <= INSTR_GEP;
}

static MAC_INLINE inline bool
instr_flow(Instr *instr)
{
	return instr->kind == INSTR_BR || instr->kind == INSTR_CONDBR || instr->kind == INSTR_RET;
}

#endif
