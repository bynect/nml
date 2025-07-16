#ifndef NML_IR_H
#define NML_IR_H

#include "nml_mac.h"
#include "nml_str.h"

typedef struct {

} Module;

typedef struct Instr {
	struct Instr *next;
} Instr;

typedef struct {
	Sized_Str name;
	Instr *instrs;
} Proto;

#endif
