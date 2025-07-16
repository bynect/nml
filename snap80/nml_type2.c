#include "nml_type2.h"
#include "nml_str.h"

Type type_unit = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("Unit"),
		.items = NULL,
		.len = 0,
	},
};

Type type_int = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("Int"),
		.items = NULL,
		.len = 0,
	},
};

Type type_float = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("Float"),
		.items = NULL,
		.len = 0,
	},
};

Type type_str = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("String"),
		.items = NULL,
		.len = 0,
	},
};

Type type_char = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("Char"),
		.items = NULL,
		.len = 0,
	},
};

Type type_bool = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("Bool"),
		.items = NULL,
		.len = 0,
	},
};

Type type_var = {
	.kind = TYPE_VAR,
	.var = {
		.name = STR_INIT("a"),
	},
};

Type type_list_ = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("List"),
		.items = &type_var,
		.len = 1,
	},
};

Type type_list = {
	.kind = TYPE_POLY,
	.poly = {
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
		.type = &type_list_,
	},
};
