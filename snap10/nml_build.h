#ifndef NML_BUILD_H
#define NML_BUILD_H

#include "nml_mem.h"
#include "nml_mac.h"
#include "nml_ir.h"

typedef struct {
	Slot var;
	Address addr;
	Label label;
	Function *fun;
} Builder;

void builder_init(Builder *build, Function *fun);

MAC_HOT Variable builder_local(Builder *build);

MAC_HOT Variable builder_address(Builder *build);

MAC_HOT Variable builder_global(Builder *build, Sized_Str name);

#endif
