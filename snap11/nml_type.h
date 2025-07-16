#ifndef NML_INFER_H
#define NML_INFER_H

#include <stdint.h>
#include <stddef.h>

#include "nml_buf.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_parse.h"
#include "nml_tab.h"
#include "nml_str.h"
#include "nml_err.h"
#include "nml_mem.h"

typedef struct Ast_Node Ast_Node;

typedef enum {
#define TAG(name, _, kind)	TAG_ ## name,
#define TYPE
#include "nml_tag.h"
#undef TYPE
#undef TAG
} Type_Tag;

#define TYPE_TAG(tag, ptr)	(tag == TAG_FUN ? (Type)ptr : ((Type)tag << 56 | (Type)ptr))
#define TYPE_UNTAG(type)	((Type_Tag)((type >> 56) & 0xff))
#define TYPE_PTR(type)		((void *)(type & ~((Type)0xff << 56)))
#define TYPE_TID(type)		((Type_Id)(type & ~((Type)0xff << 56)))

typedef uint64_t Type;
typedef uint32_t Type_Id;

typedef struct {
	Type par;
	Type ret;
} Type_Fun;

typedef struct {
	Type *items;
	size_t len;
} Type_Tuple;

#define TYPE_NONE				TYPE_TAG(TAG_NONE, NULL)
#define TYPE_UNIT				TYPE_TAG(TAG_UNIT, NULL)
#define TYPE_INT				TYPE_TAG(TAG_INT, NULL)
#define TYPE_FLOAT				TYPE_TAG(TAG_FLOAT, NULL)
#define TYPE_STR				TYPE_TAG(TAG_STR, NULL)
#define TYPE_CHAR				TYPE_TAG(TAG_CHAR, NULL)
#define TYPE_BOOL				TYPE_TAG(TAG_BOOL, NULL)

TAB_DECL(type_tab, Type, Sized_Str, Type)

TAB_DECL(subst_tab, Subst, Type_Id, Type)

typedef struct {
	Type t1;
	Type t2;
} Type_Constr;

#define TYPE_CONSTR(t1, t2)		((Type_Constr) {(t1), (t2)})

BUF_DECL(constr_buf, Constr, Type_Constr)

typedef struct {
	Hash_Default seed;
	Type_Id tid;
	Arena *arena;
	Error_List *err;
	bool succ;
	Type_Tab *ctx;
} Type_Checker;

void checker_init(Type_Checker *check, Arena *arena, Hash_Default seed, Type_Tab *ctx);

MAC_HOT bool checker_infer(Type_Checker *check, Ast_Top *ast, Error_List *err);

#endif
