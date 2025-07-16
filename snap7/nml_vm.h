#ifndef NML_VM_H
#define NML_VM_H

#include "nml_val.h"
#include "nml_mac.h"

typedef enum {
#define INSTR(name, _)	INSTR_ ## name,
#define VM
#include "nml_instr.h"
#undef VM
#undef INSTR
} Virtual_Instr;

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
} Virtual_Reg;

typedef struct {
	uint8_t stack[4096 * 4];
} Virtual_Machine;

void vm_init(Virtual_Machine *vm);

MAC_HOT Value vm_eval(Virtual_Machine *vm, const uint8_t *code);

#endif
