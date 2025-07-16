#include "nml_type2.h"
#include "nml_str.h"

// Intrinsic types

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

Type type_list = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("List"),
		.items = &type_var,
		.len = 1,
	},
};

Type type_poly_list = {
	.kind = TYPE_POLY,
	.poly = {
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
		.type = &type_list,
	},
};

Type type_int_fun = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_int,
		.ret = &type_int,
	},
};

Type type_int_int_fun = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_int,
		.ret = &type_int_fun,
	},
};

Type type_float_fun = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_float,
		.ret = &type_float,
	},
};

Type type_float_float_fun = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_float,
		.ret = &type_float_fun,
	},
};

Type type_bool_fun = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_bool,
		.ret = &type_bool,
	},
};

Type type_bool_bool_fun = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_bool,
		.ret = &type_bool_fun,
	},
};

Type type_cmp = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_var,
		.ret = &(Type) {
			.kind = TYPE_ARROW,
			.arrow = {
				.par = &type_bool,
				.ret = &type_bool_fun,
			},
		},
	},
};

Type type_poly_cmp = {
	.kind = TYPE_POLY,
	.poly = {
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
		.type = &type_cmp,
	},
};

Type type_panic = {
	.kind = TYPE_ARROW,
	.arrow = {
		.par = &type_str,
		.ret = &type_var,
	},
};

Type type_poly_panic = {
	.kind = TYPE_POLY,
	.poly = {
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
		.type = &type_cmp,
	},
};
