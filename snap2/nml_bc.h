#ifndef NML_BC_H
#define NML_BC_H

#include <stddef.h>
#include <stdint.h>

#include "nml_buf.h"

typedef enum {
#define INSTR(name, _)	INSTR_ ## name,
#define BC
#include "nml_instr.h"
#undef BC
#undef INSTR
} Instr;

typedef enum {
	REG_V0,
	REG_V1,
	REG_V2,
	REG_V3,
	REG_V4,
	REG_V5,
	REG_V6,
	REG_V7,
	REG_V8,
	REG_V9,
	REG_V10,
	REG_V11,
	REG_V12,
	REG_V13,
	REG_V14,
	REG_V15,
	REG_C0,
	REG_C1,
	REG_C2,
	REG_C3,
	REG_MAX,
} Register;

typedef struct {
	const uint8_t *code;
	size_t len;
} Bytecode;

#endif
