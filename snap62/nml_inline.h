#ifndef NML_INLINE_H
#define NML_INLINE_H

#include "nml_ast.h"
#include "nml_hash.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_set.h"

typedef struct {
	Arena *arena;
	Str_Set set;
} Inliner;

void inliner_init(Inliner *inlin, Arena *arena, Hash_Default seed);

void inliner_free(Inliner *inlin);

MAC_HOT void inliner_pass(Inliner *inlin, Ast_Top *ast);

#endif
