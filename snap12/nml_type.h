#ifndef NML_INFER_H
#define NML_INFER_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"

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

#endif
