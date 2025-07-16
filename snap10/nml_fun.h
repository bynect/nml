#ifndef NML_FUN_H
#define NML_FUN_H

#include "nml_ir.h"
#include "nml_mac.h"

typedef uint32_t Block_Id;

typedef struct Block {
	Block_Id id;
	Instr *entry;
	struct Block *next;
} Block;

typedef struct {
	Type ret;
	Type *pars;
	size_t len;
	Block *entry;
} Function;

#endif
