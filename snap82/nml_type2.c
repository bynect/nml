#include "nml_type2.h"
#include "nml_mac.h"
#include "nml_str.h"

#define CONSTR_NEW(_name)								\
	&(const Type) {										\
		.kind = TYPE_CONSTR,							\
		.constr = {										\
			.name = STR_INIT(_name),					\
			.items = NULL,								\
			.len = 0,									\
		},												\
	}



const Type *const type_unit = &(const Type) {
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
		.items = (Type*[]){&type_var},
		.len = 1,
	},
};

Type type_poly_list = {
	.kind = TYPE_POLY,
	.poly = {
		.type = &type_list,
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
	},
};

Type type_opt = {
	.kind = TYPE_CONSTR,
	.constr = {
		.name = STR_INIT("Option"),
		.items = (Type*[]){&type_var},
		.len = 1,
	},
};

Type type_opt_list = {
	.kind = TYPE_POLY,
	.poly = {
		.type = &type_opt,
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
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
		.type = &type_cmp,
		.vars = (Sized_Str[]) {STR_INIT("a")},
		.len = 1,
	},
};


MAC_HOT Type *
type_arrow_new(Arena *arena, Type *par, Type *ret)
{
	Type *arrow = arena_alloc(arena, sizeof(Type));
	arrow->kind = TYPE_VAR;
	arrow->arrow.par = par;
	arrow->arrow.ret = ret;
	return arrow;
}

MAC_HOT Type *
type_tuple_new(Arena *arena, Type **items, size_t len)
{
	Type *tuple = arena_alloc(arena, sizeof(Type));
	tuple->kind = TYPE_VAR;
	tuple->tuple.items = items;
	tuple->tuple.len = len;
	return tuple;
}

MAC_HOT Type *
type_var_new(Arena *arena, Sized_Str name)
{
	Type *var = arena_alloc(arena, sizeof(Type));
	var->kind = TYPE_VAR;
	var->var.name = name;
	return var;
}

MAC_HOT Type *
type_constr_new(Arena *arena, Sized_Str name, Type **items, size_t len)
{
	Type *constr = arena_alloc(arena, sizeof(Type));
	constr->kind = TYPE_CONSTR;
	constr->constr.name = name;
	constr->constr.items = items;
	constr->constr.len = len;
	return constr;
}

MAC_HOT Type *
type_poly_new(Arena *arena, Sized_Str name, Type **items, size_t len)
{
	Type *constr = arena_alloc(arena, sizeof(Type));
	constr->kind = TYPE_CONSTR;
	constr->constr.name = name;
	constr->constr.items = items;
	constr->constr.len = len;
	return constr;
}
