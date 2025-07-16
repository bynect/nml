#include "nml_build.h"
#include "nml_mac.h"

void
builder_init(Builder *build, Function *fun)
{
	build->var = 0;
	build->addr = 0;
	build->label = 0;
	build->fun = fun;
}

MAC_HOT void
builder_instr(Builder *build)
{
	//TODO for various type of instr
}

MAC_HOT Variable
builder_local(Builder *build)
{
	return (Variable) {
		.kind = VAR_ADDR,
		.var = build->var++,
	};
}

MAC_HOT Variable
builder_address(Builder *build)
{
	return (Variable) {
		.kind = VAR_ADDR,
		.addr = build->addr++,
	};
}

MAC_HOT Variable
builder_global(Builder *build, Sized_Str name)
{
	return (Variable) {
		.kind = VAR_GLOB,
		.glob = name,
	};
}
