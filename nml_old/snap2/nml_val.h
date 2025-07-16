#ifndef NML_VAL_H
#define NML_VAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nml_mac.h"

typedef uint64_t Value;

#define VAL_BITOFF(n)		(((n) - 1) * 8)
#define VAL_BITSET(val, n)	(((val) & ((Value)1 << (n))) != 0)

#define VAL_MASK(n)			((Value)1 << (n))
#define VAL_UNMASK(val, n)	(((Value)(val) >> (n)) & 0xff)

#define VAL_SIGNMASK		VAL_MASK(63)
#define VAL_NANMASK			((Value)0x7ffc000000000000)
#define VAL_NANTAG(val, tag) \
	(((val) & (VAL_NANMASK | (tag))) == (VAL_NANMASK | (tag)))

typedef struct Box Box;

#define VAL_ISBOX(val)		VAL_NANTAG(val, VAL_SIGNMASK)
#define VAL_BOX(box)		((Value)(VAL_NANMASK | (Value)(uintptr_t)(box) | VAL_SIGNMASK))
#define VAL_UNBOX(val)		((Box *)(uintptr_t)((val) & ~(VAL_NANMASK | VAL_SIGNMASK)))

typedef enum {
#define TAG(tag, _, kind)	VAL_TAG_ ## tag = VAL_MASK(VAL_BITOFF(6) + __COUNTER__),
#define VALUE
#include "nml_tag.h"
#undef VALUE
#undef TAG
} Value_Tag;

#define VAL_UNIT			((Value)(VAL_NANMASK | VAL_TAG_UNIT))
#define VAL_TRUE			((Value)(VAL_NANMASK | VAL_TAG_TRUE))
#define VAL_FALSE			((Value)(VAL_NANMASK | VAL_TAG_FALSE))

#define VAL_ISUNIT(val)		((Value)(val) == VAL_UNIT)
#define VAL_ISTRUE(val)		((Value)(val) == VAL_TRUE)
#define VAL_ISFALSE(val)	((Value)(val) == VAL_FALSE)

#define VAL_ISINT(val)		(value_is_int(val))
#define VAL_INT(int)		((Value)(VAL_NANMASK | VAL_TAG_INT | (int)))
#define VAL_UNINT(val)		((int32_t)((Value)(val) & 0xffffffff))

#define VAL_ISFLOAT(val)	(((val) & VAL_NANMASK) != VAL_NANMASK)
#define VAL_FLOAT(flt)		(value_from_float(flt))
#define VAL_UNFLOAT(val)	(value_to_float(val))

#define VAL_ISCHAR(val)		(value_is_char(val))
#define VAL_CHAR(chr)		((Value)(VAL_NANMASK | VAL_TAG_CHAR | chr))
#define VAL_UNCHAR(val)		((char)((Value)(val) & 0xff))

#define VAL_ISBOOL(val)		(value_is_bool(val))
#define VAL_BOOL(bool)		((bool) ? VAL_TRUE : VAL_FALSE)
#define VAL_UNBOOL(val)		((val) == VAL_TRUE)

static MAC_INLINE inline bool
value_is_int(Value val)
{
	return !VAL_BITSET(val, 63) && VAL_NANTAG(val, VAL_TAG_INT);
}

static MAC_INLINE inline bool
value_is_char(Value val)
{
	return !VAL_BITSET(val, 63) && VAL_NANTAG(val, VAL_TAG_CHAR);
}

static MAC_INLINE inline bool
value_is_bool(Value val)
{
	return VAL_ISTRUE(val) || VAL_ISFALSE(val);
}

static MAC_INLINE inline Value
value_from_float(double flt)
{
	union {
		double flt;
		Value val;
	} cast = {.flt = flt};
	return cast.val;
}

static MAC_INLINE inline double
value_to_float(Value val)
{
	union {
		double flt;
		Value val;
	} cast = {.val = val};
	return cast.flt;
}

#endif
