#ifndef NML_FUN_H
#define NML_FUN_H

#include "nml_ir.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_block.h"

typedef struct {
	size_t len;
	Block *head;
	Block *tail;
} Function;

void function_init(Function *fun, size_t len);

MAC_HOT void function_append(Function *fun, Block *block);

#endif
