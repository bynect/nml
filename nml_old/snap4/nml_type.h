#ifndef NML_TYPE_H
#define NML_TYPE_H

#include <stdint.h>
#include <stddef.h>

#include "nml_str.h"
#include "nml_tab.h"
#include "nml_parse.h"
#include "nml_hash.h"
#include "nml_err.h"

typedef enum {
#define TAG(tag, _, kind)	TAG_ ## tag,
#define TYPE
#include "nml_tag.h"
#undef TYPE
#undef TAG
} Type_Tag;

#define TYPE_TAG(tag, ptr)	((Type)tag << 56 | (Type)ptr)
#define TYPE_UNTAG(type)	((Type_Tag)((type >> 56) & 0xff))
#define TYPE_PTR(type)		((void *)(type & ~((Type)0xff << 56)))

typedef uint64_t Type;

typedef struct {
	Type par;
	Type ret;
} Type_Fun;

typedef struct {
	Type *items;
	size_t len;
} Type_Tuple;

typedef struct {
	Sized_Str name;
} Type_Var;

#define TYPE_NONE				TYPE_TAG(TAG_NONE, NULL)
#define TYPE_UNIT				TYPE_TAG(TAG_UNIT, NULL)
#define TYPE_INT				TYPE_TAG(TAG_INT, NULL)
#define TYPE_FLOAT				TYPE_TAG(TAG_FLOAT, NULL)
#define TYPE_STR				TYPE_TAG(TAG_STR, NULL)
#define TYPE_CHAR				TYPE_TAG(TAG_CHAR, NULL)
#define TYPE_BOOL				TYPE_TAG(TAG_BOOL, NULL)

typedef struct {
	const Sized_Str *vars;
	size_t len;
	Type type;
} Type_Scheme;

TAB_DECL(subst_tab, Subst, Sized_Str, Type)

TAB_DECL(scheme_tab, Scheme, Sized_Str, Type_Scheme)

typedef struct {
	Hash_Default seed;
	uint32_t tid;
	Arena *arena;
	Error_List *err;
	bool succ;
	Scheme_Tab *ctx;
} Type_Checker;

void checker_init(Type_Checker *check, Arena *arena, Hash_Default seed, Scheme_Tab *ctx);

MAC_HOT bool checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err);

#endif
