#ifndef NML_FUN_H
#define NML_FUN_H

#include "nml_ir.h"
#include "nml_mac.h"
#include "nml_mem.h"

typedef struct Block {
	Block_Id id;
	Instr *head;
	Instr *tail;
	struct Block *next;
} Block;

typedef struct {
	size_t len;
	Block *head;
	Block *tail;
} Function;

void block_init(Block *block, Block_Id id, Instr *entry);

MAC_HOT void block_append(Block *block, Instr *instr);

void function_init(Function *fun, size_t len);

void function_append(Function *fun, Block *block);

#endif
