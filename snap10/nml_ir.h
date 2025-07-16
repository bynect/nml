#ifndef NML_IR_H
#define NML_IR_H

#include "nml_const.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_str.h"
#include "nml_sym.h"
#include "nml_type.h"

typedef uint32_t Slot;
typedef uint32_t Label;
typedef uint32_t Address;

typedef enum {
	VAR_LOC,
	VAR_ADDR,
	VAR_GLOB,
} Var_Kind;

typedef struct {
	Var_Kind kind;
	union {
		Slot var;
		Address addr;
		Sized_Str glob;
	};
} Variable;

typedef enum {
#define INSTR(name, _) INSTR_ ## name,
#include "nml_instr.h"
#undef INSTR
} Instr_Kind;

typedef struct Instr {
	Instr_Kind kind;
	union {
		struct {
			Variable m_rhs;
			Variable lhs;
			Variable yield;
		} op;
		struct {
			Type type;
			Address addr;
			Variable yield;
		} load;
		struct {
			Address addr;
			Variable value;
		} store;
		struct {
			Type type;
			Address yield;
		} alloc;
		struct {
			Type type;
			Address addr;
			uint32_t index;
			Variable yield;
		} elemof;
		struct {
			Variable value;
			Address yield;
		} addrof;
		struct {
			Variable m_cond;
			Label then;
			Label m_else;
		} br;
		struct {
			Variable value;
		} ret;
		struct {
			Variable fun;
			Variable *args;
			size_t len;
			Variable yield;
		} call;
	};
	struct Instr *next;
} Instr;

typedef struct Block {
	Slot_Id id;
	Instr *entry;
} Block;


typedef enum {
	GLOB_FUN,
	GLOB_CONST,
} Glob_Kind;

typedef struct Global {
	Glob_Kind kind;
	Sized_Str name;
	union {
		Function *fun;
		Const_Value value;
	};
	struct Global *next;
} Global;

#endif
