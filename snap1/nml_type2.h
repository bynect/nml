#ifndef NML_INFER_H
#define NML_INFER_H

#include <stdint.h>
#include <stddef.h>

#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_parse.h"
#include "nml_tab.h"
#include "nml_str.h"
#include "nml_err.h"
#include "nml_mem.h"

typedef struct Ast_Node Ast_Node;

typedef enum {
#define TAG(tag, _, kind) TAG_ ## tag,
#define TYPEPTR
#define TYPE
#include "nml_tag.h"
#undef TYPEPTR
#undef TYPE
#undef TAG
} Type_Tag;

#define TYPE_TAG(tag, ptr)	((Type)tag << 56 | (Type)ptr)
#define TYPE_UNTAG(type)	((Type_Tag)((type >> 56) & 0xff))
#define TYPE_PTR(type)		((void *)(type & ~((Type)0xff << 56)))

typedef uint64_t Type;

typedef struct {
	Type *pars;
	size_t len;
	Type ret;
} Type_Fun;

typedef struct {
	Type *items;
	size_t len;
} Type_Tuple;

typedef struct {
	Sized_Str name;
} Type_Var;

typedef struct {
	Sized_Str name;
} Type_Binder;

#define TYPE_NONE				TYPE_TAG(TAG_NONE, NULL)
#define TYPE_UNIT				TYPE_TAG(TAG_UNIT, NULL)
#define TYPE_INT				TYPE_TAG(TAG_INT, NULL)
#define TYPE_FLOAT				TYPE_TAG(TAG_FLOAT, NULL)
#define TYPE_STR				TYPE_TAG(TAG_STR, NULL)
#define TYPE_CHAR				TYPE_TAG(TAG_CHAR, NULL)
#define TYPE_BOOL				TYPE_TAG(TAG_BOOL, NULL)

//typedef struct {
//	Sized_Str *vars;
//	size_t len;
//	Type type;
//} Type_Scheme;
//
//#define SCHEME_EMPTY(type)		(Type_Scheme) { NULL, 0, (type) }

TAB_DECL(type_tab, Type, Sized_Str, Type)

//TAB_DECL(scheme_tab, Scheme, Sized_Str, Type_Scheme)

typedef struct {
	Hash_Default seed;
	uint64_t tid;
	Arena *arena;
	Error_List *err;
	Type_Tab *ctx;
} Type_Checker;

void checker_init(Type_Checker *check, Arena *arena, Hash_Default seed, Type_Tab *ctx);

MAC_HOT bool checker_infer_ast(Type_Checker *check, Ast_Top *ast, Error_List *err);

MAC_HOT bool checker_infer_node(Type_Checker *check, Ast_Node *node, Error_List *err);

#endif
