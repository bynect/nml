#ifndef NML_VM_H
#define NML_VM_H

#include "nml_val.h"
#include "nml_mac.h"
#include "nml_bc.h"

typedef struct {
	uint8_t stack[4096 * 4];
} Virtual_Machine;

void vm_init(Virtual_Machine *vm);

MAC_HOT Value vm_eval(Virtual_Machine *vm, const uint8_t *code);

#endif
