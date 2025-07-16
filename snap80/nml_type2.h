#ifndef NML_TYPE_H
#define NML_TYPE_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_arena.h"
#include "nml_str.h"

typedef enum {
	TYPE_NONE,
	TYPE_VAR,
	TYPE_ARROW,
	TYPE_TUPLE,
	TYPE_CONSTR,
	// TODO:
	// TYPE_UNIVAR, // Check this
	TYPE_POLY, // Introduce polymorphism, use this instead of schemes
} Type_Kind;

typedef struct Type {
	Type_Kind kind;
	union {
		struct {
			Sized_Str name;
		} var;
		struct {
			struct Type *par;
			struct Type *ret;
		} arrow;
		struct {
			struct Type *items;
			size_t len;
		} tuple;
		struct {
			Sized_Str name;
			struct Type *items;
			size_t len;
		} constr;
		struct {
			Sized_Str *vars;
			size_t len;
			struct Type *type;
		} poly;
	};
} Type;

//typedef enum {
//	TYPE_FUN,
//	TYPE_TUPLE,
//	TYPE_CONSTR,
//} Type_Kind;
//
//typedef struct Type {
//	Type_Kind kind;
//	union {
//		struct {
//			struct Type *par;
//			struct Type *ret;
//		} fun;
//		struct {
//			struct Type *items;
//			size_t len;
//		} tuple;
//		struct {
//			Sized_Str name;
//		} var;
//		struct {
//			Sized_Str name;
//			struct Type *items;
//			size_t len;
//		} constr;
//	};
//} Type;
//
//extern Type type_unit;
//
//extern Type type_int;
//
//extern Type type_float;
//
//extern Type type_str;
//
//extern Type type_char;
//
//extern Type type_bool;
//
//typedef struct {
//	Type type;
//	Sized_Str *vars;
//	size_t len;
//} Type_Scheme;
//
//#define SCHEME_EMPTY(type)			((Type_Scheme) {type, NULL, 0})

//static MAC_INLINE inline Type
//type_fun_new(Arena *arena, Type par, Type ret)
//{
//	Type_Fun *fun = arena_alloc(arena, sizeof(Type_Fun));
//	fun->par = par;
//	fun->ret = ret;
//	return TYPE_TAG(TAG_FUN, fun);
//}
//
//static MAC_INLINE inline Type
//type_tuple_new(Arena *arena, Type *items, size_t len)
//{
//	Type_Tuple *tuple = arena_alloc(arena, sizeof(Type_Tuple));
//	tuple->items = items;
//	tuple->len = len;
//	return TYPE_TAG(TAG_TUPLE, tuple);
//}
//
//static MAC_INLINE inline Type
//type_var_new(Arena *arena, Sized_Str name)
//{
//	Type_Var *var = arena_alloc(arena, sizeof(Type_Var));
//	var->var = name;
//	return TYPE_TAG(TAG_VAR, var);
//}
//
//static MAC_INLINE inline Type
//type_constr_new(Arena *arena, Sized_Str name, Type *items, size_t len)
//{
//	Type_Constr *constr = arena_alloc(arena, sizeof(Type_Constr));
//	constr->name = name;
//	constr->items = items;
//	constr->len = len;
//	return TYPE_TAG(TAG_CONSTR, constr);
//}

#endif
