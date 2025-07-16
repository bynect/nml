#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>

#include "bytebuf.h"
#include "value.h"

typedef enum {
	OP_NOP,
	OP_RET,
} Op_Code;

typedef struct {
	Byte_Buf buf;
	Value_Vec consts;
} Byte_Code;

#endif
