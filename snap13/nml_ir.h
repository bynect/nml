#ifndef NML_IR_H
#define NML_IR_H

#include "nml_const.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_type.h"


typedef uint32_t Slot_Id;

typedef enum {
	IR_VOID,
	IR_PTR,
	IR_FUN,
	IR_REF,
	IR_INT,
	IR_FLT,
} Ir_Type_Kind;

typedef struct Ir_Type {
	Ir_Type_Kind kind;
	union {
		struct Ir_Type *ptr;
		struct {
			struct Ir_Type *ptr;
		} fun;
		Slot_Id id;
		uint32_t width;
	};
} Ir_Type;

typedef struct {
	Sized_Str name;
	Slot_Id id;
} Global;

typedef struct {
	Slot_Id id;
} Local;

typedef enum {
	VAL_GLOB,
	VAL_LOC,
} Value_Kind;

typedef struct {
	Value_Kind kind;
	Ir_Type type;
	union {
		Global glob;
		Local loc;
	};
} Value;




typedef uint32_t Var_Id;

typedef enum {
	VAR_LOC,
	VAR_ADDR,
	VAR_GLOB,
} Var_Kind;

typedef struct {
	Var_Kind kind;
	union {
		Var_Id id;
		Sized_Str name;
	};
} Variable;

#define VAR_ERR		((Variable) {.kind = VAR_LOC, .id = 0})
#define VAR_STRGLOB(str)	((Variable) {.kind = VAR_GLOB, .name = str})

typedef uint32_t Block_Id;

#define BLOCK_ERR	0

typedef enum {
#define INSTR(name, _) INSTR_ ## name,
#include "nml_instr.h"
#undef INSTR
} Instr_Kind;

typedef struct {
	Variable lhs;
	Variable yield;
} Instr_Unary;

typedef struct {
	Variable rhs;
	Variable lhs;
	Variable yield;
} Instr_Binary;

typedef struct {
	Variable addr;
	Variable yield;
} Instr_Load;

typedef struct {
	Variable addr;
	Variable value;
} Instr_Store;

typedef struct {
	Variable yield;
} Instr_Alloc;

typedef struct {
	Variable addr;
	uint32_t index;
	Variable yield;
} Instr_Elemof;

typedef struct {
	Variable value;
	Variable yield;
} Instr_Addrof;

typedef struct {
	Variable cond;
	Block_Id b_then;
	Block_Id b_else;
} Instr_Br;

typedef struct {
	Variable value;
} Instr_Ret;

typedef struct {
	Variable fun;
	Variable *args;
	size_t len;
	Variable yield;
} Instr_Call;

typedef struct Instr {
	Instr_Kind kind;
	Type type;
	union {
		Instr_Unary unary;
		Instr_Binary binary;
		Instr_Load load;
		Instr_Store store;
		Instr_Alloc alloc;
		Instr_Elemof elemof;
		Instr_Addrof addrof;
		Instr_Br br;
		Instr_Ret ret;
		Instr_Call call;
	};
	struct Instr *next;
} Instr;

MAC_HOT Instr *instr_unary_new(Arena *arena, Instr_Kind kind, Type type,
						Variable lhs, Variable yield);

MAC_HOT Instr *instr_binary_new(Arena *arena, Instr_Kind kind, Type type,
						Variable rhs, Variable lhs, Variable yield);

MAC_HOT Instr *instr_load_new(Arena *arena, Type type, Variable addr, Variable yield);

MAC_HOT Instr *instr_store_new(Arena *arena, Type type, Variable addr, Variable value);

MAC_HOT Instr *instr_alloc_new(Arena *arena, Type type, Variable yield);

MAC_HOT Instr *instr_elemof_new(Arena *arena, Type type, Variable addr,
						uint32_t index, Variable yield);

MAC_HOT Instr *instr_addrof_new(Arena *arena, Type type, Variable value, Variable yield);

MAC_HOT Instr *instr_br_new(Arena *arena, Type type, Variable cond,
					Block_Id b_then, Block_Id b_else);

MAC_HOT Instr *instr_ret_new(Arena *arena, Type type, Variable value);

MAC_HOT Instr *instr_call_new(Arena *arena, Type type, Variable fun,
						Variable *args, size_t len, Variable yield);

#endif
