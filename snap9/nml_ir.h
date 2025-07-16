#ifndef NML_IR_H
#define NML_IR_H

#include "nml_mac.h"
#include "nml_str.h"

typedef enum {
#define INSTR(name, _) INSTR_ ## name,
#define IR
#include "nml_instr.h"
#undef IR
#undef INSTR
} Instr;

typedef struct {

} Module;

typedef struct {
	Sized_Str name;
	Instr *instrs;
} Proto;

#endif
