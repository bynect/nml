#ifndef NML_TYPE_H
#define NML_TYPE_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_arena.h"
#include "nml_str.h"

// TODO: How to represent NONE/ERROR types and how to optimize code usage and sharing of types memory
// Additionally how should type schemes/polymorphic types be represented?
// How should variables quantified by forall be represented?
//
// Should generalization based on level be used? (like in OCaml)

typedef enum {
	TYPE_VAR,
	TYPE_ARROW,
	TYPE_TUPLE,
	TYPE_CONSTR,
	// TODO:
	// TYPE_UNIVAR, // Check this
	TYPE_POLY, // Forall variables quantifier that introduce polymorphism, use this instead of schemes
} Type_Kind;

typedef struct Type {
	Type_Kind kind;
	union {
		struct {
			Sized_Str name;
			//uint32_t level; // Check this
		} var;
		struct {
			struct Type *par;
			struct Type *ret;
		} arrow;
		struct {
			struct Type **items;
			size_t len;
		} tuple;
		struct {
			Sized_Str name;
			struct Type **items;
			size_t len;
		} constr;
		struct {
			struct Type *type;
			Sized_Str *vars;
			size_t len;
		} poly;
	};
} Type;

//extern Type *const type_unit;
//
//extern Type *const type_int;
//
//extern Type *const type_float;
//
//extern Type *const type_str;
//
//extern Type *const type_char;
//
//extern Type *const type_bool;
//
//extern Type *const type_poly_list;
//
//extern Type *const type_poly_panic;

MAC_HOT Type *type_arrow_new(Arena *arena, Type *par, Type *ret);

MAC_HOT Type *type_tuple_new(Arena *arena, Type **items, size_t len);

MAC_HOT Type *type_var_new(Arena *arena, Sized_Str name);

MAC_HOT Type *type_constr_new(Arena *arena, Sized_Str name, Type **items, size_t len);

MAC_HOT Type *type_poly_new(Arena *arena, Sized_Str name, Type **items, size_t len);

#endif
