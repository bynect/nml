#ifndef NML_ERR_H
#define NML_ERR_H

#include <stddef.h>
#include <stdint.h>

#include "nml_str.h"
#include "nml_mem.h"

typedef enum {
	ERR_ERROR = 1 << 0,
	ERR_WARN = 1 << 1,
	ERR_INFO = 1 << 2,
} Error_Flag;

typedef struct Error {
	Sized_Str msg;
	uint16_t line;
	Error_Flag flags;
	struct Error *next;
} Error;

typedef struct {
	Error *start;
	Error *last;
} Error_List;

typedef void (*Error_Iter_Fn)(Error *err, void *ctx);

void error_init(Error_List *err);

MAC_HOT void error_append(Error_List *err, Arena *arena, Sized_Str msg, uint16_t line, Error_Flag flags);

void error_iter(Error_List *err, Error_Iter_Fn fn, void *ctx);

#endif
